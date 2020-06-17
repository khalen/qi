#include "basictypes.h"
#include "game.h"
#include "memory.h"
#include "sound.h"
#include "debug.h"
#include "hwi.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#if HAS(OSX_BUILD)
#include <sys/mman.h>
#include <dlfcn.h>
#include <libproc.h>
#include <unistd.h>
#endif
#include <errno.h>

#undef internal
#if HAS(OSX_BUILD)
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "hw_ogl.h"
#include "iconfont.h"

#if HAS(WIN32_BUILD)
#include <direct.h>
#define PATH_MAX MAX_PATH
#endif

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_events.h"
#include "SDL_scancode.h"
#include "SDL_video.h"
#include "SDL_clipboard.h"
#include "SDL_mouse.h"
#include "SDL_stdinc.h"

#include "imgui.h"
#include "bitmap.h"

#define internal static

#define NUM_AUDIO_BUFFERS 2
struct Sound_s
{
};

// All the globals!
struct Globals_s
{
	bool gameRunning;

	Bitmap         frameBuffer;
	Sound_s        sound;
	SoundBuffer_s *soundUpdateBuffer;
	Input          inputState;
	Memory         memory;
	u32            cursorBuf[16];
	u32            curCursorPos;

	char gameAppPath[PATH_MAX];
	char gameRecordBasePath[PATH_MAX];

	void *             gameDylib;
	char               gameDylibPath[PATH_MAX];
	i32                gameDylibLastMtime;
	const GameFuncs_s *game;

	FILE *loopingFile;
	i32   recordingChannel;
	i32   playbackChannel;
	bool  isLooping;

	r64 timeConversionFactor;

	ThreadContext thread;

	bool mouseDown[MOUSE_BUTTON_COUNT];

	// Heaps for SDL and ImGUI
	static const size_t sdlHeapSize   = 150000;
	static const size_t imGuiHeapSize = 150000;

	BuddyAllocator *sdlAllocator;
	BuddyAllocator *imGuiAllocator;

	// IMGUI
	SDL_Cursor *  mouseCursors[ImGuiMouseCursor_COUNT];
	SDL_Window *  window;
	char *        clipboardTextData;
	ImGuiContext *imGuiContext;
} g;

#define NOTE_UNUSED(x) ((void)x);

void Qi_Assert_Handler(const char *msg, const char *file, const int line)
{
#if HAS(OSX_BUILD)
	asm("int $3");
#else
	__debugbreak();
#endif
}

// Probably a much less dnry way to do this, but
static void *SdlMalloc(size_t size)
{
	return BA_Alloc(g.sdlAllocator, size);
}

static void *SdlCalloc(size_t num, size_t size)
{
	return BA_Calloc(g.sdlAllocator, num * size);
}

static void *SdlRealloc(void *mem, size_t newSize)
{
	return BA_Realloc(g.sdlAllocator, mem, newSize);
}

static void SdlFree(void *mem)
{
	BA_Free(g.sdlAllocator, mem);
}

static void *ImGuiMalloc(size_t size, void *)
{
	return BA_Alloc(g.imGuiAllocator, size);
}

static void ImGuiFree(void *mem, void *)
{
	return BA_Free(g.imGuiAllocator, mem);
}

static char *StringCopy(char *dest, size_t destSize, const char *src)
{
	Assert(dest && src && destSize < UINT_MAX);

	size_t chrPos = 0;
	while (*src && chrPos < destSize - 1)
	{
		dest[chrPos] = src[chrPos];
		chrPos++;
	}

	dest[chrPos] = 0;
	return dest;
}

// Note: byte length, not string length ie. doesn't parse utf8
static size_t StringLen(const char *const src)
{
	Assert(src);

	const char *cur = src;
	while (*cur)
		cur++;
	return cur - src;
}

// Note: NOT strncat() semantics!! dstTotalSize is the total size of the destination buffer, not
// (as in strncat) the -remaining- size.
static char *StringAppend(char *dest, size_t dstTotalSize, const char *src)
{
	Assert(dest && src && dstTotalSize > 0);

	const char *const lastValidDstPos = dest + dstTotalSize - 1;
	char *            curDestPos      = dest + StringLen(dest);
	while (*src && curDestPos < lastValidDstPos)
		*curDestPos++ = *src++;
	*curDestPos = 0;
	return dest;
}

static void FindGameAppPath()
{
#if HAS(OSX_BUILD)
	// Set up module path
	pid_t pid = getpid();
	proc_pidpath(pid, g.gameAppPath, sizeof(g.gameAppPath));
	size_t fileNameLen = StringLen(g.gameAppPath);
#else
	// Set up module path
	DWORD fileNameLen = GetModuleFileNameA(0, g.gameAppPath, sizeof(g.gameAppPath));
#endif
	char *pastLastSlash = g.gameAppPath;
	for (u32 i = 0; i < fileNameLen; i++)
		if (g.gameAppPath[i] == '\\' || g.gameAppPath[i] == '/')
			pastLastSlash = g.gameAppPath + i + 1;
	*pastLastSlash = 0;
}

static void MakeGameEXERelativePath(char *dst, const char *filename)
{
	StringCopy(dst, PATH_MAX, g.gameAppPath);
	StringAppend(dst, PATH_MAX, filename);
}

static void OS_UpdateSound() {}

static r64 WallSeconds()
{
	return (r64)SDL_GetPerformanceCounter() * g.timeConversionFactor;
}

static void OS_LoadGameDll()
{
	struct stat statbuf;
	if (stat(g.gameDylibPath, &statbuf) != 0)
	{
		fprintf(stderr, "Couldn't find game dylib: %s\n", g.gameDylibPath);
		return;
	}

#if HAS(OSX_BUILD)
	if (statbuf.st_mtimespec.tv_sec == g.gameDylibLastMtime)
		return;
	g.gameDylibLastMtime = statbuf.st_mtimespec.tv_sec;
#else
	if (statbuf.st_mtime == g.gameDylibLastMtime)
		return;
	g.gameDylibLastMtime = statbuf.st_mtime;
#endif

	if (g.gameDylib != nullptr)
	{
		SDL_UnloadObject(g.gameDylib);
		g.gameDylib = nullptr;
	}

	printf("Hotloading game dylib\n");

	g.gameDylib = SDL_LoadObject(g.gameDylibPath);

	if (g.gameDylib)
	{
		typedef const GameFuncs_s *GetGameFuncs_f();
		GetGameFuncs_f *           GetGameFuncs = (GetGameFuncs_f *)SDL_LoadFunction(g.gameDylib, "Qi_GetGameFuncs");
		Assert(GetGameFuncs);
		g.game = GetGameFuncs();

		Assert(g.game->Init);
		g.game->Init(plat, &g.memory);
	}
	else
	{
		fprintf(stderr, "Couldn't dlopen game dylib: %s\n", g.gameDylibPath);
	}
}

// Game platform service functions
static r64 OS_WallSeconds(ThreadContext *)
{
	return WallSeconds();
}

static void *OS_ReadEntireFile(ThreadContext *, const char *fileName, size_t *fileSize)
{
	struct stat statBuf;
	if (stat(fileName, &statBuf) != 0)
	{
		Assert(0 && "Couldn't stat a shader file");
	}

	void *fileBuf = malloc(statBuf.st_size + 1);
	memset(fileBuf, 0, statBuf.st_size + 1);
	Assert(fileBuf);

	FILE *f = fopen(fileName, "rb");
	fread(fileBuf, 1, statBuf.st_size, f);
	fclose(f);

	if (fileSize != nullptr)
		*fileSize = statBuf.st_size;

	return fileBuf;
}

static bool OS_WriteEntireFile(ThreadContext *, const char *fileName, const void *ptr, const size_t size)
{
	FILE * outFile   = fopen(fileName, "wb");
	size_t wroteSize = fwrite(ptr, 1, size, outFile);
	Assert(wroteSize == size);
	fclose(outFile);
	return true;
}

static void OS_ReleaseFileBuffer(ThreadContext *, void *buffer)
{
	free(buffer);
}

static void OS_SetupMainExeLibraries()
{
	ImGui::SetCurrentContext(g.imGuiContext);
	// ImGui::SetAllocatorFunctions(ImGuiMalloc, ImGuiFree);
}

static ImGuiContext *OS_GetGuiContext()
{
	return g.imGuiContext;
}

static const char *ImGuiGetClipboardText(void *)
{
	if (g.clipboardTextData)
		SDL_free(g.clipboardTextData);
	SDL_GetClipboardText();
	g.clipboardTextData = SDL_GetClipboardText();
	return g.clipboardTextData;
}

static void ImGuiSetClipboardText(void *, const char *text)
{
	SDL_SetClipboardText(text);
}

static struct FontDef
{
	const char *name;
	i32         size;
	i32         xAdd;
	i32         yOffset;
} s_fontDefs[] = {
	{"Cousine-Regular.ttf", 14, 0, 0},
	{"DejaVuSans.ttf", 14, 0, 0},
	{"DejaVuSansMono.ttf", 14, 0, 0},
	{"DroidSans.ttf", 14, 0, 0},
	{"Karla-Regular.ttf", 14, 0, 0},
	{"ProggyClean.ttf", 13, 0, 1},
	{"ProggyTiny.ttf", 10, 0, 1},
	{"Roboto-Medium.ttf", 14, 0, 0},
};

static void InitGuiFonts()
{
	int i;
	auto& io = ImGui::GetIO();

	io.Fonts->AddFontDefault();

	char fontPath[PATH_MAX];

	// TODO: HAS(EDITOR) define
	// Add hardcoded Sweet16 font
	ImFontConfig ssconfig;
	ssconfig.OversampleH = 1;
	ssconfig.OversampleV = 1;
	ssconfig.PixelSnapH  = true;
	ssconfig.SizePixels  = 16;
	io.Fonts->AddFontFromFileTTF("fonts/Sweet16Mono.ttf", 16, &ssconfig);

	ssconfig.GlyphExtraSpacing = ImVec2(1, 0);
	io.Fonts->AddFontFromFileTTF("fonts/Sweet16.ttf", 16, &ssconfig);

	ssconfig.MergeMode                = true;
	ssconfig.GlyphMinAdvanceX         = 16.0f; // Use if you want to make the icon monospaced
	ssconfig.OversampleH              = 1;
	ssconfig.OversampleV              = 1;
	ssconfig.GlyphExtraSpacing        = ImVec2(0, 0);
	static const ImWchar iconRanges[] = {ICON_MIN_FK, ICON_MAX_FK, 0};
	io.Fonts->AddFontFromFileTTF("fonts/forkawesome-webfont.ttf", 16.0f, &ssconfig, iconRanges);

	for (i = 0; i < sizeof(s_fontDefs) / sizeof(s_fontDefs[0]); i++)
	{
		const FontDef* def = &s_fontDefs[i];

		ImFontConfig cfg;
		cfg.OversampleH = 2;
		cfg.OversampleV = 1;
		cfg.GlyphExtraSpacing = ImVec2(def->xAdd, 0);
		snprintf(fontPath, sizeof(fontPath),"fonts/%s", def->name);
		ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, def->size, &cfg);
		font->DisplayOffset.y += def->yOffset;
	}
}

static void InitImGui(SDL_Window *window)
{
	g.window = window;

	if (g.imGuiContext == nullptr)
	{
		g.imGuiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(g.imGuiContext);
	}

	ImGuiIO &io = ImGui::GetIO();

	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor() values (optional)
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;  // We can honor io.WantSetMousePos requests (optional, rarely used)
	io.BackendPlatformName = "QI Engine";

	// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
	io.KeyMap[ImGuiKey_Tab]         = SDL_SCANCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow]   = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow]  = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow]     = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow]   = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp]      = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown]    = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home]        = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End]         = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert]      = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete]      = SDL_SCANCODE_DELETE;
	io.KeyMap[ImGuiKey_Backspace]   = SDL_SCANCODE_BACKSPACE;
	io.KeyMap[ImGuiKey_Space]       = SDL_SCANCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter]       = SDL_SCANCODE_RETURN;
	io.KeyMap[ImGuiKey_Escape]      = SDL_SCANCODE_ESCAPE;
	io.KeyMap[ImGuiKey_KeyPadEnter] = SDL_SCANCODE_KP_ENTER;
	io.KeyMap[ImGuiKey_A]           = SDL_SCANCODE_A;
	io.KeyMap[ImGuiKey_C]           = SDL_SCANCODE_C;
	io.KeyMap[ImGuiKey_V]           = SDL_SCANCODE_V;
	io.KeyMap[ImGuiKey_X]           = SDL_SCANCODE_X;
	io.KeyMap[ImGuiKey_Y]           = SDL_SCANCODE_Y;
	io.KeyMap[ImGuiKey_Z]           = SDL_SCANCODE_Z;

	io.SetClipboardTextFn = ImGuiSetClipboardText;
	io.GetClipboardTextFn = ImGuiGetClipboardText;
	io.ClipboardUserData  = nullptr;

	// Load mouse cursors
	g.mouseCursors[ImGuiMouseCursor_Arrow]      = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	g.mouseCursors[ImGuiMouseCursor_TextInput]  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	g.mouseCursors[ImGuiMouseCursor_ResizeAll]  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	g.mouseCursors[ImGuiMouseCursor_ResizeNS]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	g.mouseCursors[ImGuiMouseCursor_ResizeEW]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	g.mouseCursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	g.mouseCursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	g.mouseCursors[ImGuiMouseCursor_Hand]       = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	g.mouseCursors[ImGuiMouseCursor_NotAllowed] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

	InitGuiFonts();
}

static void InitGameGlobals()
{
	g.timeConversionFactor = 1.0 / (double)SDL_GetPerformanceFrequency();

#if HAS(DEV_BUILD)
	size_t baseAddressPerm  = TB(2);
	size_t baseAddressTrans = TB(4);
#else
	size_t baseAddressPerm  = 0;
	size_t baseAddressTrans = 0;
#endif

	g.memory.permanentSize = MB(64);
	g.memory.transientSize = GB(4);

#if HAS(OSX_BUILD)
	g.memory.permanentStorage = (u8 *)mmap((void *)baseAddressPerm, g.memory.permanentSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	if (g.memory.permanentStorage == (void *)MAP_FAILED)
	{
		fprintf(stderr, "mmap failed with code: %d\n", errno);
		exit(1);
	}
	Assert(g.memory.permanentStorage != nullptr);
	g.memory.permanentPos = g.memory.permanentStorage;

	g.memory.transientSize    = GB(4);
	g.memory.transientStorage = (u8 *)mmap((void *)baseAddressTrans, g.memory.transientSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	if (g.memory.transientStorage == (void *)MAP_FAILED)
	{
		fprintf(stderr, "mmap failed with code: %d\n", errno);
		exit(1);
	}
	Assert(g.memory.transientStorage != nullptr);
	g.memory.transientPos = g.memory.transientStorage;
#else
	g.memory.permanentStorage = (u8 *)VirtualAlloc((void *)baseAddressPerm, g.memory.permanentSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(g.memory.permanentStorage != nullptr);
	g.memory.permanentPos = g.memory.permanentStorage;

	g.memory.transientStorage = (u8 *)VirtualAlloc((void *)baseAddressTrans, g.memory.transientSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(g.memory.transientStorage != nullptr);
	g.memory.transientPos = g.memory.transientStorage;
#endif

	// Set up heaps for SDL and ImGUI
#if 0
	g.sdlAllocator = BA_Init(&g.memory, g.sdlHeapSize, 16, false);
	g.imGuiAllocator = BA_Init(&g.memory, g.imGuiHeapSize, 16, false);
#endif

	const char *gameLibSuffix = "";

#if HAS(OSX_BUILD)
	const char *gameDylibName = "libqi_game.dylib";
#else
	const char *gameDylibName = "qi_game.dll";
#endif

	FindGameAppPath();
	MakeGameEXERelativePath(g.gameDylibPath, gameDylibName);
	MakeGameEXERelativePath(g.gameRecordBasePath, "qi_");

	OS_LoadGameDll();

	// Assert(g.game && g.game->sound);
	Assert(g.game);
#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
	// Assert(g.game->debug);
#endif

	g.frameBuffer.width    = GAME_RES_X;
	g.frameBuffer.height   = GAME_RES_Y;
	g.frameBuffer.pitch    = (g.frameBuffer.width + 0xF) & ~0xF;
	size_t fbSize          = g.frameBuffer.pitch * g.frameBuffer.height * sizeof(u32);
	g.frameBuffer.byteSize = fbSize;
#if HAS(OSX_BUILD)
	g.frameBuffer.pixels = (u32 *)mmap(0, fbSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	Assert(g.frameBuffer.pixels != (u32 *)MAP_FAILED);
#else
	g.frameBuffer.pixels = (u32 *)VirtualAlloc(nullptr, fbSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(g.frameBuffer.pixels != nullptr);
#endif
	for (u32 c = 0; c < g.frameBuffer.pitch / sizeof(u32) * g.frameBuffer.height; c++)
		g.frameBuffer.pixels[c] = 0xFF00FFFF;

	g.frameBuffer.hardwareId = nullptr;
	Hwi* hwi = g.game->GetHwi();
	hwi->SetScreenTarget(&g.frameBuffer);
}

static const char *IntToString(const i32 src)
{
	static char result[12];
	char        tmpBuf[12]; // Max length of an int in ascii + room for minus sign + null terminator

	i32        tmpPos     = 0;
	const bool isNegative = (src < 0);

	i32 absSrc = src & 0x7FFFFFFF;
	while (absSrc >= 10)
	{
		tmpBuf[tmpPos++] = (absSrc % 10) + '0';
		absSrc /= 10;
	}
	tmpBuf[tmpPos] = (char)(absSrc + '0');

	if (isNegative)
		tmpBuf[++tmpPos] = '-';

	i32 resultPos = 0;

	for (resultPos = 0; tmpPos >= 0; tmpPos--, resultPos++)
		result[resultPos] = tmpBuf[tmpPos];
	result[resultPos] = 0;

	return result;
}

#define LOOP_FILE_EXT ".rec"

static const char *GetLoopingChannelFileName(i32 channel)
{
	static char loopFile[PATH_MAX];
	StringCopy(loopFile, sizeof(loopFile), g.gameRecordBasePath);
	StringAppend(loopFile, sizeof(loopFile), IntToString(channel));
	StringAppend(loopFile, sizeof(loopFile), LOOP_FILE_EXT);
	return loopFile;
}

void BeginPlayback(i32 channel)
{
	Assert(channel > 0);
	Assert(!g.playbackChannel);
	Assert(!g.recordingChannel);
	Assert(!g.loopingFile);

	g.loopingFile = fopen(GetLoopingChannelFileName(channel), "rb");
	if (g.loopingFile == nullptr)
		return;

	size_t memSize;
	if (fread(&memSize, 1, sizeof(size_t), g.loopingFile) != sizeof(size_t))
	{
		fclose(g.loopingFile);
		g.loopingFile = nullptr;
		fprintf(stderr, "Couldn't read size intro block from looping file %s\n", GetLoopingChannelFileName(channel));
		return;
	}

	if (fread(g.memory.permanentStorage, 1, memSize, g.loopingFile) != memSize)
	{
		fprintf(stderr, "Failed to read memory block on playback\n");
		fclose(g.loopingFile);
		g.loopingFile = nullptr;
		return;
	}
	g.playbackChannel = channel;
}

void EndPlayback()
{
	Assert(g.playbackChannel > 0);
	fclose(g.loopingFile);
	g.loopingFile     = nullptr;
	g.playbackChannel = 0;
}

bool PlaybackInput(Input *input)
{
	size_t bytesRead = 0;
	bytesRead        = fread(input, 1, sizeof(*input), g.loopingFile);
	if (bytesRead != sizeof(*input))
	{
		i32 curChannel = g.playbackChannel;
		EndPlayback();
		BeginPlayback(curChannel);
		if (!PlaybackInput(input))
		{
			EndPlayback();
		}
		else
		{
			return true;
		}
	}
	else
	{
		return true;
	}

	return false;
}

void BeginRecording(i32 channel)
{
	Assert(channel > 0);
	Assert(!g.recordingChannel);

	// Support going from looping straight back to recording
	if (g.loopingFile != nullptr)
	{
		fclose(g.loopingFile);
		g.loopingFile     = nullptr;
		g.playbackChannel = 0;
	}

	Assert(g.loopingFile == nullptr);
	g.loopingFile = fopen(GetLoopingChannelFileName(channel), "wb");
	if (g.loopingFile != nullptr)
	{
		size_t permSize     = (size_t)(g.memory.permanentPos - g.memory.permanentStorage);
		size_t bytesWritten = 0;
		bytesWritten        = fwrite(&permSize, 1, sizeof(size_t), g.loopingFile);
		Assert(bytesWritten == sizeof(permSize));
		bytesWritten = fwrite(g.memory.permanentStorage, 1, permSize, g.loopingFile);
		Assert(bytesWritten == permSize);
	}

	g.recordingChannel = channel;
}

void RecordInput(const Input *input)
{
	Assert(g.recordingChannel > 0 && g.loopingFile != nullptr);
	size_t bytesWritten = 0;
	bytesWritten        = fwrite(input, 1, sizeof(*input), g.loopingFile);
	Assert(bytesWritten == sizeof(*input));
}

void EndRecording()
{
	Assert(g.recordingChannel > 0 && g.loopingFile != nullptr);
	fclose(g.loopingFile);
	g.loopingFile      = nullptr;
	g.recordingChannel = 0;
}

static void ProcessKeyboardStick(Analog *analog, r32 valueX, r32 valueY, bool isDown)
{
	v2 incDir;
	incDir.x = isDown ? valueX : -valueX;
	incDir.y = isDown ? valueY : -valueY;

	// This is gross
	v2 oldDir = analog->dir;
	oldDir.x  = oldDir.x < 0.0f ? -1.0f : oldDir.x > 0.0f ? 1.0f : 0.0f;
	oldDir.y  = oldDir.y < 0.0f ? -1.0f : oldDir.y > 0.0f ? 1.0f : 0.0f;

	VAdd(&analog->dir, &oldDir, &incDir);
	VNormalize(&analog->dir);
	analog->trigger.reading  = VLength(&analog->dir);
	analog->trigger.minMax.x = analog->trigger.minMax.y = analog->trigger.reading;
}

static void ProcessButton(Button *button, bool isDown)
{
	button->halfTransitionCount++;
	button->endedDown = isDown;
}

static void HandleMiscEvent(SDL_Event *, Input *) {}

static void HandleMouseEvent(SDL_Event *event, Input *newInput)
{
	// Handle IMGui stuff
	ImGuiIO &io = ImGui::GetIO();

	switch (event->type)
	{
	case SDL_MOUSEWHEEL:
		if (event->wheel.x > 0)
			io.MouseWheelH += 1.0f;
		if (event->wheel.x < 0)
			io.MouseWheelH -= 1.0f;
		if (event->wheel.y > 0)
			io.MouseWheel += 1.0f;
		if (event->wheel.y < 0)
			io.MouseWheel -= 1.0f;
		break;
	case SDL_MOUSEBUTTONDOWN:
		Assert(event->button.button - 1 >= 0 && event->button.button - 1 < MOUSE_BUTTON_COUNT);
		g.mouseDown[event->button.button - 1] = true;
		break;
	case SDL_MOUSEBUTTONUP:
		Assert(event->button.button - 1 >= 0 && event->button.button - 1 < MOUSE_BUTTON_COUNT);
		g.mouseDown[event->button.button - 1] = false;
		break;
	}

	if (io.WantCaptureMouse)
		return;

	// Handle our stuff
	switch (event->type)
	{
	case SDL_MOUSEBUTTONDOWN:
		ProcessButton(&newInput->mouse.buttons[event->button.button - 1], true);
		break;
	case SDL_MOUSEBUTTONUP:
		ProcessButton(&newInput->mouse.buttons[event->button.button - 1], false);
		break;
	}
}

static void UpdateMousePosAndButtons(Input *newInput)
{
	ImGuiIO &io = ImGui::GetIO();

	int mx, my;
	u32 buttons = SDL_GetMouseState(&mx, &my);
	for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
	{
		io.MouseDown[i] = g.mouseDown[i] || (buttons & SDL_BUTTON(i + 1));
		g.mouseDown[i]  = false;
	}
	// Because SDL mouse 1 is middle click and ImGui mouse 1 is right click
	bool argh       = io.MouseDown[2];
	io.MouseDown[2] = io.MouseDown[1];
	io.MouseDown[1] = argh;

	io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
	if (SDL_GetWindowFlags(g.window) & SDL_WINDOW_INPUT_FOCUS)
		io.MousePos = ImVec2((float)mx, (float)my);
}

static void UpdateMouseCursor()
{
	ImGuiIO &io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
		return;

	ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
	if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None)
	{
		SDL_ShowCursor(SDL_FALSE);
	}
	else
	{
		SDL_SetCursor(g.mouseCursors[imguiCursor] ? g.mouseCursors[imguiCursor] : g.mouseCursors[ImGuiMouseCursor_Arrow]);
		SDL_ShowCursor(SDL_TRUE);
	}
}

static void HandleKeyEvent(SDL_Event *event, Input *newInput)
{
	bool        isDown        = (event->key.type == SDL_KEYDOWN);
	Controller *kbdController = &newInput->controllers[KBD];

	SDL_Keycode vkCode = event->key.keysym.sym;

	const int modState   = SDL_GetModState();
	bool      commandKey = (modState & KMOD_GUI);
	bool      shiftKey   = (modState & KMOD_SHIFT);
	bool      altKey     = (modState & KMOD_ALT);
	bool      ctrlKey    = (modState & KMOD_CTRL);

	// Handle all IMGUI stuff
	ImGuiIO &io = ImGui::GetIO();

	if (event->type == SDL_TEXTINPUT)
	{
		io.AddInputCharactersUTF8(event->text.text);
		return;
	}

	const int key = event->key.keysym.scancode;
	Assert(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));

	io.KeysDown[key] = isDown;
	io.KeyShift      = shiftKey;
	io.KeyCtrl       = ctrlKey;
	io.KeyAlt        = altKey;
	io.KeySuper      = commandKey;

	if (io.WantCaptureKeyboard)
		return;

	ProcessButton(&kbdController->leftStickButton, shiftKey);

	if (vkCode == SDLK_w)
	{
		ProcessKeyboardStick(&kbdController->leftStick, 0.0f, 1.0f, isDown);
	}
	else if (vkCode == SDLK_d)
	{
		ProcessKeyboardStick(&kbdController->leftStick, 1.0f, 0.0f, isDown);
	}
	else if (vkCode == SDLK_s)
	{
		ProcessKeyboardStick(&kbdController->leftStick, 0.0f, -1.0f, isDown);
	}
	else if (vkCode == SDLK_a)
	{
		ProcessKeyboardStick(&kbdController->leftStick, -1.0f, 0.0f, isDown);
	}
	else if (vkCode == SDLK_q)
	{
		if (isDown && commandKey)
			g.gameRunning = false;
		else
			ProcessButton(&kbdController->leftShoulder, isDown);
	}
	else if (vkCode == SDLK_e && ctrlKey && isDown)
	{
		g.game->ToggleEditor();
	}
	else if (vkCode == SDLK_e)
	{
		ProcessButton(&kbdController->rightShoulder, isDown);
	}
	else if (vkCode == SDLK_UP)
	{
		ProcessButton(&kbdController->upButton, isDown);
	}
	else if (vkCode == SDLK_RIGHT)
	{
		ProcessButton(&kbdController->rightButton, isDown);
	}
	else if (vkCode == SDLK_DOWN)
	{
		ProcessButton(&kbdController->downButton, isDown);
	}
	else if (vkCode == SDLK_LEFT)
	{
		ProcessButton(&kbdController->leftButton, isDown);
	}
	else if (vkCode == SDLK_ESCAPE)
	{
		g.gameRunning = false;
	}
	else if (vkCode == SDLK_SPACE)
	{
	}
	else if (vkCode == SDLK_l && isDown)
	{
		if (g.recordingChannel > 0)
		{
			i32 curChannel = g.recordingChannel;
			EndRecording();
			BeginPlayback(curChannel);
		}
		else if (g.playbackChannel > 0)
		{
			EndPlayback();
		}
		else
		{
			BeginRecording(1);
		}
	}
}

void UpdateImGui(Input *newInput)
{
	ImGuiIO &io = ImGui::GetIO();

	// Setup display size (every frame to accommodate for window resizing)
	int w, h;
	int displayWidth, displayHeight;
	SDL_GetWindowSize(g.window, &w, &h);
	SDL_GL_GetDrawableSize(g.window, &displayWidth, &displayHeight);
	io.DisplaySize = ImVec2((float)w, (float)h);
	if (w > 0 && h > 0)
		io.DisplayFramebufferScale = ImVec2((float)displayWidth / w, (float)displayHeight / h);

	// Setup time step (we don't use SDL_GetTicks() because it is using millisecond resolution)
	static Uint64 frequency    = SDL_GetPerformanceFrequency();
	Uint64        current_time = SDL_GetPerformanceCounter();
	io.DeltaTime               = g.inputState.dT;

	UpdateMousePosAndButtons(newInput);
	UpdateMouseCursor();
}

#ifdef main
#undef main
#endif
int main(int argc, const char *argv[])
{
// In dev builds, chdir into the hardcoded data directory to facilitate running the exe from
// non distribution places
#if HAS(DEV_BUILD) && HAS(OSX_BUILD)
	chdir("/Users/jon/work/qi/data/");
#elif HAS(DEV_BUILD) && HAS(WIN32_BUILD)
	chdir("d:\\work\\qi\\data");
#endif

	// SDL_SetMemoryFunctions(SdlMalloc, SdlCalloc, SdlRealloc, SdlFree);

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Init(SDL_INIT_TIMER);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window *  window  = SDL_CreateWindow("QI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GAME_RES_X, GAME_RES_Y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	g.window = window;
	InitImGui(window);
	InitGameGlobals();

	ImGui::StyleColorsLight();

	SDL_GL_MakeCurrent(window, context);

	r64 secs = WallSeconds();
	SDL_Delay(1000);
	r64 oneSec = WallSeconds() - secs;
	printf("Onesec: %g\n", oneSec);

	r64 frameStart = WallSeconds();
	g.gameRunning  = true;

	bool showDemoWindow = false;

	while (g.gameRunning)
	{
		Input newInput = {};

		OS_LoadGameDll();

		Controller *      kbdController = &newInput.controllers[KBD];
		const Controller *oldController = &g.inputState.controllers[KBD];
		for (int button = 0; button < BUTTON_COUNT; button++)
			kbdController->buttons[button].endedDown = oldController->buttons[button].endedDown;
		for (int analog = 0; analog < ANALOG_COUNT; analog++)
			kbdController->analogs[analog] = oldController->analogs[analog];

		SDL_Event event = {0};
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
				HandleKeyEvent(&event, &newInput);
			else if (event.type >= SDL_MOUSEMOTION && event.type < SDL_JOYAXISMOTION)
				HandleMouseEvent(&event, &newInput);
			else if (event.type >= SDL_CONTROLLERAXISMOTION && event.type < SDL_FINGERDOWN)
				HandleMouseEvent(&event, &newInput);
			else
				HandleMiscEvent(&event, &newInput);
		}

		g.inputState = newInput;

		OS_UpdateSound();

		if (g.recordingChannel > 0)
			RecordInput(&g.inputState);

		if (g.playbackChannel > 0)
			PlaybackInput(&g.inputState);

		const r64 secondsPerFrame = 1.0 / TARGET_FPS;
		g.inputState.dT           = (Time_t)secondsPerFrame;

		UpdateImGui(&g.inputState);

		Hwi* hwi = g.game->GetHwi();
		hwi->BeginFrame();
		ImGui::ShowDemoWindow(&showDemoWindow);

		g.game->UpdateAndRender(&g.thread, &g.inputState, &g.frameBuffer);

		volatile r64 frameElapsed    = WallSeconds() - frameStart;
		u32          maxSoundSleepMS = QI_SOUND_REQUESTED_LATENCY_MS / NUM_AUDIO_BUFFERS;

		if (frameElapsed < secondsPerFrame)
		{
			u32 msToSleep = (u32)(1000.0 * (secondsPerFrame - frameElapsed));
			while (msToSleep > maxSoundSleepMS)
			{
				OS_UpdateSound();
				SDL_Delay(maxSoundSleepMS);
				msToSleep -= maxSoundSleepMS;
			}

			if (msToSleep > 0)
				SDL_Delay(msToSleep);

			while (frameElapsed < secondsPerFrame)
			{
				OS_UpdateSound();
				frameElapsed = WallSeconds() - frameStart;
			}
		}
		else
		{
			// TODO: Log missed frame
		}

		frameStart = WallSeconds();
		hwi->EndFrame();
		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return EXIT_SUCCESS;
}

// Interface to game DLL
static PlatFuncs_s s_plat = {
	OS_ReadEntireFile,
	OS_WriteEntireFile,
	OS_ReleaseFileBuffer,
	OS_WallSeconds,
	OS_SetupMainExeLibraries,
	OS_GetGuiContext,
};
const PlatFuncs_s *plat = &s_plat;
