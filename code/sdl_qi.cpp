#include "basictypes.h"
#include "qi.h"
#include "qi_memory.h"
#include "qi_sound.h"
#include "qi_debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <libproc.h>
#include <unistd.h>
#include <errno.h>

#undef internal
#include <mach/mach.h>
#include <mach/mach_time.h>

#include "qi_ogl.h"

#include <SDL_opengl.h>
#include <SDL.h>
#include "SDL_events.h"
#include "SDL_scancode.h"
#include "SDL_video.h"

#define internal static

#define NUM_AUDIO_BUFFERS 2
struct Sound_s
{
};

// All the globals!
struct Globals_s
{
	bool   gameRunning;
	double clockConversionFactor;

	Bitmap_s	   frameBuffer;
	Sound_s		   sound;
	SoundBuffer_s* soundUpdateBuffer;
	Input_s		   inputState;
	Memory_s	   memory;
	u32			   cursorBuf[16];
	u32			   curCursorPos;

	char gameAppPath[PATH_MAX];
	char gameRecordBasePath[PATH_MAX];

	void*			   gameDylib;
	char			   gameDylibPath[PATH_MAX];
	i32				   gameDylibLastMtime;
	const GameFuncs_s* game;

	FILE* loopingFile;
	i32	  recordingChannel;
	i32	  playbackChannel;
	bool  isLooping;

	r64 timeConversionFactor;

	ThreadContext_s thread;
} g;

#define NOTE_UNUSED(x) ((void)x);

void
Qi_Assert_Handler(const char* msg, const char* file, const int line)
{
#if HAS(OSX_BUILD)
	asm("int $3");
#else
	__debugbreak();
#endif
}

static char*
StringCopy(char* dest, size_t destSize, const char* src)
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
static size_t
StringLen(const char* const src)
{
	Assert(src);

	const char* cur = src;
	while (*cur)
		cur++;
	return cur - src;
}

// Note: NOT strncat() semantics!! dstTotalSize is the total size of the destination buffer, not
// (as in strncat) the -remaining- size.
static char*
StringAppend(char* dest, size_t dstTotalSize, const char* src)
{
	Assert(dest && src && dstTotalSize > 0);

	const char* const lastValidDstPos = dest + dstTotalSize - 1;
	char*             curDestPos      = dest + StringLen(dest);
	while (*src && curDestPos < lastValidDstPos)
		*curDestPos++ = *src++;
	*curDestPos = 0;
	return dest;
}

static void
MakeGameEXERelativePath(char* dst, const char* filename)
{
	StringCopy(dst, PATH_MAX, g.gameAppPath);
	StringAppend(dst, PATH_MAX, filename);
}

static void
OS_UpdateSound()
{
}

static r64
WallSeconds()
{
	return (r64)mach_absolute_time() * g.timeConversionFactor;
}

static void
OS_LoadGameDll()
{
	struct stat statbuf;
	if (stat(g.gameDylibPath, &statbuf) != 0)
	{
		fprintf(stderr, "Couldn't find game dylib: %s\n", g.gameDylibPath);
		return;
	}

	if (statbuf.st_mtimespec.tv_sec == g.gameDylibLastMtime)
		return;

	g.gameDylibLastMtime = statbuf.st_mtimespec.tv_sec;

	if (g.gameDylib != nullptr)
	{
		dlclose(g.gameDylib);
		g.gameDylib = nullptr;
	}

	printf("Hotloading game dylib\n");

	g.gameDylib = dlopen(g.gameDylibPath, RTLD_LAZY | RTLD_GLOBAL);

	if (g.gameDylib)
	{
		typedef const GameFuncs_s* GetGameFuncs_f();
		GetGameFuncs_f*            GetGameFuncs = (GetGameFuncs_f*)dlsym(g.gameDylib, "Qi_GetGameFuncs");
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
static r64
OS_WallSeconds(ThreadContext_s*)
{
	return WallSeconds();
}

static void*
OS_ReadEntireFile(ThreadContext_s*, const char* fileName, size_t* fileSize)
{
	struct stat statBuf;
	if (stat(fileName, &statBuf) != 0)
	{
		Assert(0 && "Couldn't stat a shader file");
	}

	void* fileBuf = malloc(statBuf.st_size + 1);
	memset(fileBuf, 0, statBuf.st_size + 1);
	Assert(fileBuf);

	FILE* f = fopen(fileName, "rb");
	fread(fileBuf, 1, statBuf.st_size, f);
	fclose(f);

	return fileBuf;
}

static bool
OS_WriteEntireFile(ThreadContext_s*, const char* fileName, const void* ptr, const size_t size)
{
	FILE* outFile = fopen(fileName, "wb");
	size_t wroteSize = fwrite(ptr, 1, size, outFile);
	Assert(wroteSize == size);
	fclose(outFile);
	return true;
}

static void
OS_ReleaseFileBuffer(ThreadContext_s*, void* buffer)
{
	free(buffer);
}

static void
InitGameGlobals()
{
	mach_timebase_info_data_t timebase;

	mach_timebase_info(&timebase);
	g.timeConversionFactor = 1.0 / 1.0e9;

#if HAS(DEV_BUILD)
	size_t baseAddressPerm  = TB(2);
	size_t baseAddressTrans = TB(4);
#else
	size_t baseAddressPerm  = 0;
	size_t baseAddressTrans = 0;
#endif

	g.memory.permanentSize = MB(64);
	g.memory.permanentStorage
		= (u8*)mmap((void*)baseAddressPerm, g.memory.permanentSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	if (g.memory.permanentStorage == (void*)MAP_FAILED)
	{
		fprintf(stderr, "mmap failed with code: %d\n", errno);
		exit(1);
	}
	Assert(g.memory.permanentStorage != nullptr);
	g.memory.permanentPos = g.memory.permanentStorage;

	g.memory.transientSize = GB(4);
	g.memory.transientStorage
		= (u8*)mmap((void*)baseAddressTrans, g.memory.transientSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
	if (g.memory.transientStorage == (void*)MAP_FAILED)
	{
		fprintf(stderr, "mmap failed with code: %d\n", errno);
		exit(1);
	}
	Assert(g.memory.transientStorage != nullptr);
	g.memory.transientPos = g.memory.transientStorage;

	// Set up module path
	pid_t pid = getpid();
	proc_pidpath(pid, g.gameAppPath, sizeof(g.gameAppPath));
	size_t fileNameLen = StringLen(g.gameAppPath);

	char* pastLastSlash = g.gameAppPath;
	for (u32 i = 0; i < fileNameLen; i++)
		if (g.gameAppPath[i] == '/')
			pastLastSlash = g.gameAppPath + i + 1;
	*pastLastSlash = 0;

	MakeGameEXERelativePath(g.gameDylibPath, "libqi_game.dylib");
	MakeGameEXERelativePath(g.gameRecordBasePath, "qi_");

	OS_LoadGameDll();

	// Assert(g.game && g.game->sound);
	Assert(g.game);
#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
	//Assert(g.game->debug);
#endif

	g.frameBuffer.width = GAME_RES_X;
	g.frameBuffer.height = GAME_RES_Y;
	g.frameBuffer.pitch = (g.frameBuffer.width + 0xF) & ~0xF;
	size_t fbSize = g.frameBuffer.pitch * g.frameBuffer.height * sizeof(u32);
	g.frameBuffer.byteSize = fbSize;
	g.frameBuffer.pixels = (u32 *)mmap(0, fbSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	Assert(g.frameBuffer.pixels != (u32 *)MAP_FAILED);
	for (u32 c = 0; c < g.frameBuffer.pitch / sizeof(u32) * g.frameBuffer.height; c++)
		g.frameBuffer.pixels[c] = 0xFF00FFFF;
}

static const char*
IntToString(const i32 src)
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

static const char*
GetLoopingChannelFileName(i32 channel)
{
	static char loopFile[PATH_MAX];
	StringCopy(loopFile, sizeof(loopFile), g.gameRecordBasePath);
	StringAppend(loopFile, sizeof(loopFile), IntToString(channel));
	StringAppend(loopFile, sizeof(loopFile), LOOP_FILE_EXT);
	return loopFile;
}

void
BeginPlayback(i32 channel)
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

void
EndPlayback()
{
	Assert(g.playbackChannel > 0);
	fclose(g.loopingFile);
	g.loopingFile     = nullptr;
	g.playbackChannel = 0;
}

bool
PlaybackInput(Input_s* input)
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

void
BeginRecording(i32 channel)
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

void
RecordInput(const Input_s* input)
{
	Assert(g.recordingChannel > 0 && g.loopingFile != nullptr);
	size_t bytesWritten = 0;
	bytesWritten        = fwrite(input, 1, sizeof(*input), g.loopingFile);
	Assert(bytesWritten == sizeof(*input));
}

void
EndRecording()
{
	Assert(g.recordingChannel > 0 && g.loopingFile != nullptr);
	fclose(g.loopingFile);
	g.loopingFile      = nullptr;
	g.recordingChannel = 0;
}

static void
ProcessKeyboardStick(Analog_s* analog, r32 valueX, r32 valueY, bool isDown)
{
	v2 incDir;
	incDir.x = isDown ? valueX : -valueX;
	incDir.y = isDown ? valueY : -valueY;

	// This is gross
	v2 oldDir = analog->dir;
	oldDir.x      = oldDir.x < 0.0f ? -1.0f : oldDir.x > 0.0f ? 1.0f : 0.0f;
	oldDir.y      = oldDir.y < 0.0f ? -1.0f : oldDir.y > 0.0f ? 1.0f : 0.0f;

	VAdd(&analog->dir, &oldDir, &incDir);
	VNormalize(&analog->dir);
	analog->trigger.reading  = VLength(&analog->dir);
	analog->trigger.minMax.x = analog->trigger.minMax.y = analog->trigger.reading;
}

static void
ProcessKeyboardButton(Button_s* button, bool isDown)
{
	button->halfTransitionCount++;
	button->endedDown = isDown;
}

static void
HandleKeyEvent(SDL_Event* event, Input_s* newInput)
{
	bool		  isDown		= (event->key.type == SDL_KEYDOWN);
	Controller_s* kbdController = &newInput->controllers[KBD];

	const Uint8* keyState	= SDL_GetKeyboardState(nullptr);
	SDL_Keycode	 vkCode		= event->key.keysym.sym;
	bool		 commandKey = keyState[SDL_SCANCODE_LGUI];
	bool		 shiftKey	= keyState[SDL_SCANCODE_LSHIFT];

	// bool    controlKey    = (modifierFlags & NSEventModifierFlagControl) != 0;
	// bool    alternateKey  = (modifierFlags & NSEventModifierFlagAlternate) != 0;

	ProcessKeyboardButton(&kbdController->leftStickButton, shiftKey);

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
			ProcessKeyboardButton(&kbdController->leftShoulder, isDown);
	}
	else if (vkCode == SDLK_e)
	{
		ProcessKeyboardButton(&kbdController->rightShoulder, isDown);
	}
	else if (vkCode == SDLK_UP)
	{
		ProcessKeyboardButton(&kbdController->upButton, isDown);
	}
	else if (vkCode == SDLK_RIGHT)
	{
		ProcessKeyboardButton(&kbdController->rightButton, isDown);
	}
	else if (vkCode == SDLK_DOWN)
	{
		ProcessKeyboardButton(&kbdController->downButton, isDown);
	}
	else if (vkCode == SDLK_LEFT)
	{
		ProcessKeyboardButton(&kbdController->leftButton, isDown);
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

int
main(int argc, const char* argv[])
{
// In dev builds, chdir into the hardcoded data directory to facilitate running the exe from
// non distribution places
#if HAS(DEV_BUILD)
	chdir("/Users/jon/work/qi/data/");
#endif

	InitGameGlobals();

	r64 secs = WallSeconds();
	usleep(1000 * 1000);
	r64 oneSec = WallSeconds() - secs;
	printf("Onesec: %g\n", oneSec);

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

	SDL_Window* window = SDL_CreateWindow("QI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GAME_RES_X, GAME_RES_Y, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext context = SDL_GL_CreateContext(window);

	QiOgl_Init();
	SDL_GL_MakeCurrent(window, context);

	r64 frameStart = WallSeconds();
	g.gameRunning  = true;

	while (g.gameRunning)
	{
		Input_s newInput = {};

		OS_LoadGameDll();

		Controller_s*       kbdController = &newInput.controllers[KBD];
		const Controller_s* oldController = &g.inputState.controllers[KBD];
		for (int button = 0; button < BUTTON_COUNT; button++)
			kbdController->buttons[button].endedDown = oldController->buttons[button].endedDown;
		for (int analog = 0; analog < ANALOG_COUNT; analog++)
			kbdController->analogs[analog] = oldController->analogs[analog];

		// SampleXInputControllers(&newInput, &g.inputState);

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				HandleKeyEvent(&event, &newInput);
				break;
			default:
				break;
				// Ignore
			}
		}

		g.inputState = newInput;

		OS_UpdateSound();

		if (g.recordingChannel > 0)
			RecordInput(&g.inputState);

		if (g.playbackChannel > 0)
			PlaybackInput(&g.inputState);

		const r64 secondsPerFrame = 1.0 / TARGET_FPS;

		g.inputState.dT = (Time_t)secondsPerFrame;
		g.game->UpdateAndRender(&g.thread, &g.inputState, &g.frameBuffer);

		r64 frameElapsed    = WallSeconds() - frameStart;
		u32 maxSoundSleepMS = QI_SOUND_REQUESTED_LATENCY_MS / NUM_AUDIO_BUFFERS;

		if (frameElapsed < secondsPerFrame)
		{
			u32 msToSleep = (u32)(1000.0 * (secondsPerFrame - frameElapsed));
			while (msToSleep > maxSoundSleepMS)
			{
				OS_UpdateSound();
				usleep(maxSoundSleepMS * 1000);
				msToSleep -= maxSoundSleepMS;
			}

			if (msToSleep > 0)
				usleep(msToSleep * 1000);

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
		QiOgl_Clear();
		QiOgl_BlitBufferToScreen(&g.frameBuffer);
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
};
const PlatFuncs_s* plat = &s_plat;
