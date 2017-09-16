#include "basictypes.h"
#include "qi.h"
#include "qi_sound.h"
#include "qi_debug.h"

#undef internal
#import <Cocoa/Cocoa.h>

#define internal static

@interface QIAppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>
@end

@implementation QIAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}

- (void) applicationWillTerminate:(id)sender
{
}

int
main(int argc, const char* argv[])
{
    // Autorelease Pool:
    // Objects declared in this scope will be automatically
    // released at the end of it, when the pool is "drained".
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    // Create a shared app instance.
    // This will initialize the global variable
    // 'NSApp' with the application instance.
    [NSApplication sharedApplication];

    //
    // Create a window:
    //

    // Style flags:
    NSUInteger windowStyle = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable;

    // Window bounds (x, y, width, height).
    NSRect windowRect = NSMakeRect(100, 100, 400, 400);
    NSWindow * window = [[NSWindow alloc] initWithContentRect:windowRect
                                          styleMask:windowStyle
                                          backing:NSBackingStoreBuffered
                                          defer:NO];
    [window autorelease];

    // Window controller:
    NSWindowController * windowController = [[NSWindowController alloc] initWithWindow:window];
    [windowController autorelease];

    // This will add a simple text view to the window,
    // so we can write a test string on it.
    NSText * textView = [[NSText alloc] initWithFrame:windowRect];
    [textView autorelease];

    [window setContentView:textView];
    [textView insertText:@"Hello OSX/Cocoa world!"];

    // TODO: Create app delegate to handle system events.
    // TODO: Create menus (especially Quit!)

    // Show window and run event loop.
    [window orderFrontRegardless];
    [NSApp run];

    [pool drain];

    return 0;
}

#if 0
struct WindowsBitmap_s
{
	Bitmap_s   bitmap;
	BITMAPINFO bmi;
};

#define XA_NUM_BUFFERS 3
#define XA_UPDATE_BUFFER_SAMPLES (u32)(QI_SOUND_SAMPLES_PER_SECOND / 1000.0f * QI_SOUND_REQUESTED_LATENCY_MS + 0.5f)
#define XA_UPDATE_BUFFER_BYTES (XA_UPDATE_BUFFER_SAMPLES * QI_SOUND_BYTES_PER_SAMPLE)

struct Sound_s
{
	IXAudio2*               xaudio;
	IXAudio2MasteringVoice* masteringVoice;
	IXAudio2SourceVoice*    sourceVoice;
	u8                      buffers[XA_NUM_BUFFERS][XA_UPDATE_BUFFER_BYTES];
	u32                     buffersSubmitted;
};

// All the globals!
struct Globals_s
{
	bool          gameRunning;
	HWND          wnd;
	LARGE_INTEGER startupTime;
	double        clockConversionFactor;
	BITMAPINFO    bmi;

	WindowsBitmap_s frameBuffer;
	Sound_s         sound;
	SoundBuffer_s*  soundUpdateBuffer;
	Input_s         inputState;
	Memory_s        memory;
	u32             cursorBuf[16];
	u32             curCursorPos;

	char gameEXEPath[MAX_PATH];
	char gameRecordBasePath[MAX_PATH];

	HMODULE            gameDLL;
	FILETIME           gameDLLLastWriteTime;
	char               gameDLLPath[MAX_PATH];
	char               gameDLLOrigPath[MAX_PATH];
	const GameFuncs_s* game;

	HANDLE loopingFile;
	i32    recordingChannel;
	i32    playbackChannel;
	bool   isLooping;

	ThreadContext_s thread;
} g;

#define NOTE_UNUSED(x) ((void)x);

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(impXInputGetState);
XINPUT_GET_STATE(XInputGetStateStub)
{
	NOTE_UNUSED(dwUserIndex);
	NOTE_UNUSED(pState);
	return ERROR_DEVICE_NOT_CONNECTED;
}
internal impXInputGetState* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(impXInputSetState);
XINPUT_SET_STATE(XInputSetStateStub)
{
	NOTE_UNUSED(dwUserIndex);
	NOTE_UNUSED(pVibration);
	return ERROR_DEVICE_NOT_CONNECTED;
}
internal impXInputSetState* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

void
Qi_Assert_Handler(const char* msg, const char* file, const int line)
{
	__debugbreak();
}

internal char*
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
internal size_t
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
internal char*
StringAppend(char* dest, size_t dstTotalSize, const char* src)
{
	Assert(dest && src && dstTotalSize > 0);

	const char* const lastValidDstPos = dest + dstTotalSize - 1;
	char*             curDestPos      = dest + StringLen(dest);
	while (*src && curDestPos < lastValidDstPos)
		*curDestPos++ = *src++;
	*curDestPos       = 0;
	return dest;
}

internal void
MakeGameEXERelativePath(char* dst, const char* filename)
{
	StringCopy(dst, M
AX_PATH, g.gameEXEPath);
	StringAppend(dst, MAX_PATH, filename);
}

internal double
WallSeconds()
{
	LARGE_INTEGER curCounter;
	i64           deltaCounter;
	QueryPerformanceCounter(&curCounter);
	deltaCounter = curCounter.QuadPart - g.startupTime.QuadPart;
	return deltaCounter * g.clockConversionFactor;
}

double
Qi_WallSeconds(ThreadContext_s*)
{
	return WallSeconds();
}

// Temporary file I/O
void*
Qi_ReadEntireFile(ThreadContext_s*, const char* fileName, size_t* fileSize)
{
	HANDLE hFile
	    = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;

	LARGE_INTEGER queriedFileSize = {};
	if (!GetFileSizeEx(hFile, &queriedFileSize))
		return nullptr;

	if (fileSize)
		*fileSize = queriedFileSize.QuadPart;

	void* fileMem = VirtualAlloc(0, queriedFileSize.QuadPart, MEM_RESERVE | MEM_COMMIT, 0);
	if (!fileMem)
		return nullptr;

	DWORD bytesRead;

	ReadFile(hFile, fileMem, queriedFileSize.LowPart, &bytesRead, nullptr);
	Assert(bytesRead == queriedFileSize.LowPart);

	CloseHandle(hFile);
	return fileMem;
}

// TODO: More robust file writes (create / delete / rename)
bool
Qi_WriteEntireFile(ThreadContext_s*, const char* fileName, const void* ptr, const size_t size)
{
	HANDLE hFile
	    = CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	DWORD bytesWritten;

	WriteFile(hFile, ptr, (DWORD)size, &bytesWritten, nullptr);

	Assert(bytesWritten == size);
	CloseHandle(hFile);

	return true;
}

void
Qi_ReleaseFileBuffer(ThreadContext_s*, void* buffer)
{
	Assert(buffer);
	VirtualFree(buffer, 0, MEM_RELEASE | MEM_DECOMMIT);
}

internal void
MakeOffscreenBitmap(WindowsBitmap_s* osb, u32 width, u32 height)
{
	memset(osb, 0, sizeof(*osb));

	osb->bitmap.width    = width;
	osb->bitmap.height   = height;
	osb->bitmap.pitch    = width;
	osb->bitmap.byteSize = width * height * sizeof(u32);
	osb->bitmap.pixels   = (u32*)VirtualAlloc(0, osb->bitmap.byteSize, MEM_COMMIT, PAGE_READWRITE);

	memset(osb->bitmap.pixels, 0, osb->bitmap.byteSize);

	osb->bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	osb->bmi.bmiHeader.biBitCount    = 32;
	osb->bmi.bmiHeader.biPlanes      = 1;
	osb->bmi.bmiHeader.biCompression = BI_RGB;
	osb->bmi.bmiHeader.biWidth       = width;
	osb->bmi.bmiHeader.biHeight      = -(int)height;
}

internal void
FreeOffscreenBitmap(WindowsBitmap_s* osb)
{
	if (osb->bitmap.pixels)
		VirtualFree(osb->bitmap.pixels, 0, MEM_RELEASE);

	memset(osb, 0, sizeof(*osb));
}

internal void
GetWindowDimen(HWND wnd, Rect_s* rect)
{
	memset(rect, 0, sizeof(*rect));
	RECT clientRect;
	GetClientRect(wnd, &clientRect);
	rect->left   = 0;
	rect->top    = 0;
	rect->width  = clientRect.right - clientRect.left;
	rect->height = clientRect.bottom - clientRect.top;
}

internal int
OnSize(HWND wnd)
{
	FreeOffscreenBitmap(&g.frameBuffer);

	Rect_s wndRect;
	GetWindowDimen(wnd, &wndRect);

	MakeOffscreenBitmap(&g.frameBuffer, wndRect.width, wndRect.height);

	return 0;
}

#define TMP_DLL_NAME "qi_game_runtime.dll"
internal void
LoadGameDLL()
{
	WIN32_FILE_ATTRIBUTE_DATA fileData = {};
	if (GetFileAttributesEx(g.gameDLLOrigPath, GetFileExInfoStandard, &fileData))
	{
		if (fileData.ftLastWriteTime.dwLowDateTime == g.gameDLLLastWriteTime.dwLowDateTime
		    && fileData.ftLastWriteTime.dwHighDateTime == g.gameDLLLastWriteTime.dwHighDateTime)
			return;

		g.gameDLLLastWriteTime = fileData.ftLastWriteTime;
	}

	if (g.gameDLL != nullptr)
	{
		FreeLibrary(g.gameDLL);
		g.gameDLL = nullptr;
		g.game    = nullptr;
		Sleep(300);
	}

	OutputDebugString("Loading Game DLL\n");

	Assert(CopyFile(g.gameDLLOrigPath, g.gameDLLPath, FALSE));

	g.gameDLL = LoadLibrary(g.gameDLLPath);

	if (g.gameDLL)
	{
		typedef const GameFuncs_s* GetGameFuncs_f();
		GetGameFuncs_f*            GetGameFuncs = (GetGameFuncs_f*)GetProcAddress(g.gameDLL, "Qi_GetGameFuncs");
		Assert(GetGameFuncs);
		g.game = GetGameFuncs();

		Assert(g.game->Init);
		g.game->Init(plat, &g.memory);
	}
}

internal void
InitGlobals()
{
	memset(&g, 0, sizeof(g));

	QueryPerformanceCounter(&g.startupTime);

	LARGE_INTEGER countsPerSecond;
	QueryPerformanceFrequency(&countsPerSecond);
	g.clockConversionFactor = 1.0 / (double)countsPerSecond.QuadPart;

	HMODULE xinputLib = LoadLibrary(XINPUT_DLL);
	if (xinputLib != nullptr)
	{
		XInputGetState_ = (impXInputGetState*)GetProcAddress(xinputLib, "XInputGetState");
		XInputSetState_ = (impXInputSetState*)GetProcAddress(xinputLib, "XInputSetState");
	}

#if HAS(DEV_BUILD)
	size_t baseAddressPerm  = TB(2);
	size_t baseAddressTrans = TB(4);
#else
	size_t baseAddressPerm  = 0;
	size_t baseAddressTrans = 0;
#endif

	g.memory.permanentSize = MB(64);
	g.memory.permanentStorage
	    = (u8*)VirtualAlloc((void*)baseAddressPerm, g.memory.permanentSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(g.memory.permanentStorage != nullptr);
	g.memory.permanentPos = g.memory.permanentStorage;

	g.memory.transientSize = GB(4);
	g.memory.transientStorage
	    = (u8*)VirtualAlloc((void*)baseAddressTrans, g.memory.transientSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Assert(g.memory.transientStorage != nullptr);
	g.memory.transientPos = g.memory.transientStorage;

	// Set up module path
	DWORD fileNameLen   = GetModuleFileNameA(0, g.gameEXEPath, sizeof(g.gameEXEPath));
	char* pastLastSlash = g.gameEXEPath;
	for (u32 i = 0; i < fileNameLen; i++)
		if (g.gameEXEPath[i] == '\\')
			pastLastSlash = g.gameEXEPath + i + 1;
	*pastLastSlash        = 0;

	MakeGameEXERelativePath(g.gameDLLOrigPath, GAME_DLL_NAME);
	MakeGameEXERelativePath(g.gameDLLPath, TMP_DLL_NAME);
	MakeGameEXERelativePath(g.gameRecordBasePath, "qi_");

	LoadGameDLL();

	Assert(g.game && g.game->sound);
#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
	Assert(g.game->debug);
#endif
}

internal void
UpdateWindow(HDC dc, WindowsBitmap_s* osb)
{
	if (!osb->bitmap.pixels)
		return;

	StretchDIBits(dc,
	              0,
	              0,
	              osb->bitmap.width,
	              osb->bitmap.height,
	              0,
	              0,
	              osb->bitmap.width,
	              osb->bitmap.height,
	              osb->bitmap.pixels,
	              &osb->bmi,
	              DIB_RGB_COLORS,
	              SRCCOPY);
}

internal void
ProcessDigitalButton(DWORD xinputButtons, u32 buttonBit, const Button_s* oldState, Button_s* newState)
{
	newState->endedDown           = (xinputButtons & buttonBit) != 0;
	newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void
ProcessTrigger(Trigger_s* newState, const u16 value, const r32 deadZone)
{
	r32 reading = value / 255.0f;
	if (reading > deadZone)
	{
		reading -= deadZone;
		reading = reading / (1.0f - deadZone);
	}
	else
	{
		reading = 0.0f;
	}

	newState->reading  = reading;
	newState->minMax.x = reading;
	newState->minMax.y = reading;
}

internal void
ProcessAnalog(Analog_s* newState, const i16 inX, const i16 inY, const r32 deadZone)
{
	r32 sqMag     = (r32)inX * inX + (r32)inY * inY;
	r32 magnitude = sqrtf(sqMag);

	if (magnitude > 32767.0f)
		magnitude = 32767.0f;

	newState->dir.x = inX / magnitude;
	newState->dir.y = inY / magnitude;
	magnitude /= 32767.0f;

	if (magnitude > deadZone)
	{
		magnitude -= deadZone;
		magnitude = magnitude / (1.0f - deadZone);
	}
	else
	{
		magnitude = 0.0f;
	}

	newState->trigger.reading  = magnitude;
	newState->trigger.minMax.x = magnitude;
	newState->trigger.minMax.y = magnitude;
}

internal void
SampleXInputControllers(Input_s* newInput, const Input_s* oldInput)
{
	for (DWORD controllerIdx = 0; controllerIdx < XUSER_MAX_COUNT; controllerIdx++)
	{
		XINPUT_STATE        controllerState;
		const Controller_s* oldState = &oldInput->controllers[controllerIdx];
		Controller_s*       newState = &newInput->controllers[controllerIdx];

		if (XInputGetState(controllerIdx, &controllerState) == ERROR_SUCCESS)
		{
			XINPUT_GAMEPAD* xpad = &controllerState.Gamepad;

			newState->isAnalog = true;

			// Buttons
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_DPAD_UP, &oldState->upButton, &newState->upButton);
			ProcessDigitalButton(
			    xpad->wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, &oldState->rightButton, &newState->rightButton);
			ProcessDigitalButton(
			    xpad->wButtons, XINPUT_GAMEPAD_DPAD_DOWN, &oldState->downButton, &newState->downButton);
			ProcessDigitalButton(
			    xpad->wButtons, XINPUT_GAMEPAD_DPAD_LEFT, &oldState->leftButton, &newState->leftButton);
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_A, &oldState->aButton, &newState->aButton);
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_B, &oldState->bButton, &newState->bButton);
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_X, &oldState->xButton, &newState->xButton);
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_Y, &oldState->yButton, &newState->yButton);
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_START, &oldState->startButton, &newState->startButton);
			ProcessDigitalButton(xpad->wButtons, XINPUT_GAMEPAD_BACK, &oldState->backButton, &newState->backButton);
			ProcessDigitalButton(
			    xpad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, &oldState->leftShoulder, &newState->leftShoulder);
			ProcessDigitalButton(
			    xpad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, &oldState->rightShoulder, &newState->rightShoulder);

			// Analog
			ProcessTrigger(&newState->leftTrigger, xpad->bLeftTrigger, TRIGGER_DEADZONE);
			ProcessTrigger(&newState->rightTrigger, xpad->bRightTrigger, TRIGGER_DEADZONE);
			ProcessAnalog(&newState->leftStick, xpad->sThumbLX, xpad->sThumbLY, ANALOG_DEADZONE);
			ProcessAnalog(&newState->rightStick, xpad->sThumbRX, xpad->sThumbRY, ANALOG_DEADZONE);
		}
		else
		{
			memset(newState, 0, sizeof(*newState));
		}
	}
}

void
ProcessKeyboardStick(Analog_s* analog, r32 valueX, r32 valueY, bool isDown)
{
	Vec2_s incDir;
	incDir.x = isDown ? valueX : -valueX;
	incDir.y = isDown ? valueY : -valueY;

	// This is gross
	Vec2_s oldDir = analog->dir;
	oldDir.x      = oldDir.x < 0.0f ? -1.0f : oldDir.x > 0.0f ? 1.0f : 0.0f;
	oldDir.y      = oldDir.y < 0.0f ? -1.0f : oldDir.y > 0.0f ? 1.0f : 0.0f;

	Vec2Add(&analog->dir, &oldDir, &incDir);
	Vec2Normalize(&analog->dir);
	analog->trigger.reading  = Vec2Length(&analog->dir);
	analog->trigger.minMax.x = analog->trigger.minMax.y = analog->trigger.reading;
}

void
ProcessKeyboardButton(Button_s* button, bool isDown)
{
	button->halfTransitionCount++;
	button->endedDown = isDown;
}

internal const char*
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

	for (resultPos        = 0; tmpPos >= 0; tmpPos--, resultPos++)
		result[resultPos] = tmpBuf[tmpPos];
	result[resultPos]     = 0;

	return result;
}

#define LOOP_FILE_EXT ".rec"

internal const char*
GetLoopingChannelFileName(i32 channel)
{
	static char loopFile[MAX_PATH];
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

	g.loopingFile = CreateFileA(
	    GetLoopingChannelFileName(channel), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (g.loopingFile == nullptr)
		return;

	DWORD memSize;
	DWORD bytesRead = 0;
	if (ReadFile(g.loopingFile, &memSize, sizeof(memSize), &bytesRead, nullptr) && bytesRead == sizeof(memSize))
	{
		ReadFile(g.loopingFile, g.memory.permanentStorage, memSize, &bytesRead, nullptr);
		if (bytesRead != memSize)
		{
			OutputDebugString("Failed to read memory block on playback\n");
			CloseHandle(g.loopingFile);
			return;
		}
	}
	g.playbackChannel = channel;
}

void
EndPlayback()
{
	Assert(g.playbackChannel > 0);
	CloseHandle(g.loopingFile);
	g.loopingFile     = nullptr;
	g.playbackChannel = 0;
}

bool
PlaybackInput(Input_s* input)
{
	DWORD bytesRead = 0;
	ReadFile(g.loopingFile, input, sizeof(*input), &bytesRead, nullptr);
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
		CloseHandle(g.loopingFile);
		g.loopingFile     = nullptr;
		g.playbackChannel = 0;
	}

	Assert(g.loopingFile == nullptr);
	g.loopingFile = CreateFileA(
	    GetLoopingChannelFileName(channel), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (g.loopingFile != nullptr)
	{
		DWORD permSize     = (DWORD)(g.memory.permanentPos - g.memory.permanentStorage);
		DWORD bytesWritten = 0;
		WriteFile(g.loopingFile, &permSize, sizeof(permSize), &bytesWritten, nullptr);
		Assert(bytesWritten == sizeof(permSize));
		WriteFile(g.loopingFile, g.memory.permanentStorage, permSize, &bytesWritten, nullptr);
		Assert(bytesWritten == permSize);
	}
	g.recordingChannel = channel;
}

void
RecordInput(const Input_s* input)
{
	Assert(g.recordingChannel > 0 && g.loopingFile != nullptr);
	DWORD bytesWritten = 0;
	WriteFile(g.loopingFile, input, sizeof(*input), &bytesWritten, nullptr);
	Assert(bytesWritten == sizeof(*input));
}

void
EndRecording()
{
	Assert(g.recordingChannel > 0 && g.loopingFile != nullptr);
	CloseHandle(g.loopingFile);
	g.loopingFile      = nullptr;
	g.recordingChannel = 0;
}

void
ProcessMouseMessage(Input_s* newInput, MSG* message)
{
	Controller_s* kbdController = &newInput->controllers[KBD];

	switch (message->message)
	{
	case WM_LBUTTONDOWN:
	{
		ProcessKeyboardButton(&kbdController->leftMouse, true);
	}
	break;
	case WM_LBUTTONUP:
	{
		ProcessKeyboardButton(&kbdController->leftMouse, false);
	}
	break;
	case WM_MBUTTONDOWN:
	{
		ProcessKeyboardButton(&kbdController->middleMouse, true);
	}
	break;
	case WM_MBUTTONUP:
	{
		ProcessKeyboardButton(&kbdController->middleMouse, false);
	}
	break;
	case WM_RBUTTONDOWN:
	{
		ProcessKeyboardButton(&kbdController->rightMouse, true);
	}
	break;
	case WM_RBUTTONUP:
	{
		ProcessKeyboardButton(&kbdController->rightMouse, false);
	}
	break;
	case WM_MOUSEMOVE:
	{
		newInput->mouse.x = (r32)GET_X_LPARAM(message->lParam);
		newInput->mouse.y = (r32)GET_Y_LPARAM(message->lParam);
	}
	break;
	default:
		break;
	}
}

void
ProcessKeyboardMessage(Input_s* newInput, MSG* message)
{
	Controller_s* kbdController = &newInput->controllers[KBD];

	switch (message->message)
	{
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		u32  vkCode  = (u32)message->wParam;
		bool wasDown = (message->lParam & (1 << 30)) != 0;
		bool isDown  = (message->lParam & (1 << 31)) == 0;

		if (wasDown == isDown)
			break;

		if (vkCode == 'W')
		{
			ProcessKeyboardStick(&kbdController->leftStick, 0.0f, 1.0f, isDown);
		}
		else if (vkCode == 'D')
		{
			ProcessKeyboardStick(&kbdController->leftStick, 1.0f, 0.0f, isDown);
		}
		else if (vkCode == 'S')
		{
			ProcessKeyboardStick(&kbdController->leftStick, 0.0f, -1.0f, isDown);
		}
		else if (vkCode == 'A')
		{
			ProcessKeyboardStick(&kbdController->leftStick, -1.0f, 0.0f, isDown);
		}
		else if (vkCode == 'Q')
		{
			ProcessKeyboardButton(&kbdController->leftShoulder, isDown);
		}
		else if (vkCode == 'E')
		{
			ProcessKeyboardButton(&kbdController->rightShoulder, isDown);
		}
		else if (vkCode == VK_UP)
		{
			ProcessKeyboardButton(&kbdController->upButton, isDown);
		}
		else if (vkCode == VK_RIGHT)
		{
			ProcessKeyboardButton(&kbdController->rightButton, isDown);
		}
		else if (vkCode == VK_DOWN)
		{
			ProcessKeyboardButton(&kbdController->downButton, isDown);
		}
		else if (vkCode == VK_LEFT)
		{
			ProcessKeyboardButton(&kbdController->leftButton, isDown);
		}
		else if (vkCode == VK_ESCAPE)
		{
			g.gameRunning = false;
		}
		else if (vkCode == VK_SPACE)
		{
		}
		else if (vkCode == 'L' && isDown)
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
	break;
	}
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch (uMsg)
	{
	case WM_SIZE:
	{
		return OnSize(hwnd);
		break;
	}
	case WM_CLOSE:
	{
		g.gameRunning = false;
		break;
	}
	case WM_DESTROY:
	{
		g.gameRunning = false;
		break;
	}
	case WM_ACTIVATEAPP:
	{
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT paint;
		HDC         dc = BeginPaint(hwnd, &paint);
		UpdateWindow(dc, &g.frameBuffer);
		EndPaint(hwnd, &paint);
		break;
	}
	case WM_QUIT:
	{
		break;
	}
	default:
	{
		result = DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
	return result;
}

internal void
InitXAudio2(void)
{
	if (FAILED(XAudio2Create(&g.sound.xaudio)))
	{
		OutputDebugString("Failed to initialize XAudio2\n");
		return;
	}

#if HAS(DEV_BUILD) && 0
	XAUDIO2_DEBUG_CONFIGURATION dbgOpts = {};
	dbgOpts.TraceMask                   = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
	dbgOpts.BreakMask                   = XAUDIO2_LOG_ERRORS;
	g.sound.xaudio->SetDebugConfiguration(&dbgOpts, 0);
#endif

	if (FAILED(g.sound.xaudio->CreateMasteringVoice(&g.sound.masteringVoice)))
	{
		OutputDebugString("Failed to create mastering voice\n");
		return;
	}

	WAVEFORMATEX waveFormat    = {};
	waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
	waveFormat.nChannels       = QI_SOUND_CHANNELS;
	waveFormat.nSamplesPerSec  = QI_SOUND_SAMPLES_PER_SECOND;
	waveFormat.nAvgBytesPerSec = QI_SOUND_BYTES_PER_SAMPLE * QI_SOUND_SAMPLES_PER_SECOND;
	waveFormat.nBlockAlign     = QI_SOUND_BYTES_PER_SAMPLE;
	waveFormat.wBitsPerSample  = (QI_SOUND_BYTES_PER_SAMPLE / QI_SOUND_CHANNELS) * 8;
	waveFormat.cbSize          = 0;

	if (FAILED(
	        g.sound.xaudio->CreateSourceVoice(&g.sound.sourceVoice, &waveFormat, 0, 1.0f, nullptr, nullptr, nullptr)))
	{
		OutputDebugString("Failed to create source voice\n");
		return;
	}

	g.sound.sourceVoice->Start(0, 0);
}

internal void
Win32UpdateSound()
{
	static r64 lastTime = 0.0;
	if (lastTime == 0.0)
	{
		lastTime = WallSeconds();
	}
	else
	{
		r64 curTime   = WallSeconds();
		u32 elapsedMs = (u32)((curTime - lastTime) * 1000.0);
#if HAS(DEV_BUILD)
		if (elapsedMs > QI_SOUND_REQUESTED_LATENCY_MS)
		{
			char msg[1024];
			snprintf(msg,
			         sizeof(msg),
			         "Missed audio update, requested latency %d ms since last call %d\n",
			         QI_SOUND_REQUESTED_LATENCY_MS,
			         elapsedMs);
			OutputDebugString(msg);
		}
#else
		Assert(elapsedMs < QI_SOUND_REQUESTED_LATENCY_MS);
#endif
		lastTime = curTime;
	}

	XAUDIO2_VOICE_STATE xstate;
	g.sound.sourceVoice->GetState(&xstate, XAUDIO2_VOICE_NOSAMPLESPLAYED);
	i32 bufsToQueue = XA_NUM_BUFFERS - xstate.BuffersQueued;

	XAUDIO2_BUFFER xaBuf = {};
	xaBuf.AudioBytes     = XA_UPDATE_BUFFER_BYTES;

	if (g.soundUpdateBuffer == nullptr)
		g.soundUpdateBuffer = g.game->sound->MakeBuffer(
		    &g.memory, XA_UPDATE_BUFFER_SAMPLES, QI_SOUND_CHANNELS, QI_SOUND_SAMPLES_PER_SECOND);

	for (i32 buf = 0; buf < bufsToQueue; buf++)
	{
		i32 bufIdx    = g.sound.buffersSubmitted % XA_NUM_BUFFERS;
		u8* curBuffer = &g.sound.buffers[bufIdx][0];

		g.game->sound->Update(g.soundUpdateBuffer, XA_UPDATE_BUFFER_SAMPLES);
		memcpy(curBuffer, g.soundUpdateBuffer->samples, XA_UPDATE_BUFFER_BYTES);

		xaBuf.pAudioData = curBuffer;
		g.sound.sourceVoice->SubmitSourceBuffer(&xaBuf);
		g.sound.buffersSubmitted++;
	}
}

int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	InitGlobals();

	WNDCLASS wndClass      = {};
	wndClass.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.hInstance     = instance;
	wndClass.lpszClassName = "QIClass";
	wndClass.lpfnWndProc   = WindowProc;

	RegisterClass(&wndClass);

	DWORD wndStyle = (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~(WS_MAXIMIZE | WS_SIZEBOX);
	RECT  wndRect  = {50, 50, 50 + GAME_RES_X, 50 + GAME_RES_Y};
	AdjustWindowRect(&wndRect, wndStyle, FALSE);

	g.wnd = CreateWindowExA(0,
	                        "QIClass",
	                        "QI",
	                        wndStyle,
	                        wndRect.left,
	                        wndRect.top,
	                        wndRect.right - wndRect.left,
	                        wndRect.bottom - wndRect.top,
	                        nullptr,
	                        nullptr,
	                        instance,
	                        nullptr);

	if (g.wnd)
	{
		InitXAudio2();
		timeBeginPeriod(1);

		HDC dc = GetDC(g.wnd);

		r64 frameStart = WallSeconds();

		u32 fpsTicks[5];
		u32 cps = (u32)(1024.0f / TARGET_FPS * 2);
		for (int i = 0; i < 5; i++)
		{
			fpsTicks[i] = i * cps;
		}

		g.gameRunning = true;
		while (g.gameRunning)
		{
			Input_s newInput = {};

			LoadGameDLL();

			Controller_s*       kbdController = &newInput.controllers[KBD];
			const Controller_s* oldController = &g.inputState.controllers[KBD];
			for (int button                              = 0; button < BUTTON_COUNT; button++)
				kbdController->buttons[button].endedDown = oldController->buttons[button].endedDown;
			for (int analog                    = 0; analog < ANALOG_COUNT; analog++)
				kbdController->analogs[analog] = oldController->analogs[analog];

			SampleXInputControllers(&newInput, &g.inputState);

			MSG message;
			while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
					g.gameRunning = false;

				if (message.message >= WM_MOUSEFIRST && message.message <= WM_MOUSELAST)
				{
					ProcessMouseMessage(&newInput, &message);
					continue;
				}

				switch (message.message)
				{
				case WM_SYSKEYUP:
				case WM_SYSKEYDOWN:
				case WM_KEYUP:
				case WM_KEYDOWN:
				{
					ProcessKeyboardMessage(&newInput, &message);
					continue;
				}
				default:
				{
					TranslateMessage(&message);
					DispatchMessage(&message);
				}
				}
			}

			g.inputState = newInput;

			Win32UpdateSound();

			if (g.recordingChannel > 0)
				RecordInput(&g.inputState);

			if (g.playbackChannel > 0)
				PlaybackInput(&g.inputState);

			const r64 secondsPerFrame = 1.0 / TARGET_FPS;

			g.inputState.dT = (Time_t)secondsPerFrame;
			g.game->UpdateAndRender(&g.thread, &g.inputState, &g.frameBuffer.bitmap);

#if HAS(DEV_BUILD) || HAS(PROF_BUILD)
			g.game->debug->DrawTicks(&g.thread, &g.frameBuffer.bitmap, fpsTicks, countof(fpsTicks), 1024, 10, 50, 0);
#endif

			r64 frameElapsed    = WallSeconds() - frameStart;
			u32 maxSoundSleepMS = QI_SOUND_REQUESTED_LATENCY_MS / XA_NUM_BUFFERS;

			if (frameElapsed < secondsPerFrame)
			{
				u32 msToSleep = (u32)(1000.0 * (secondsPerFrame - frameElapsed));
				while (msToSleep > maxSoundSleepMS)
				{
					Win32UpdateSound();
					Sleep((DWORD)maxSoundSleepMS);
					msToSleep -= maxSoundSleepMS;
				}

				if (msToSleep > 0)
					Sleep(msToSleep);

				while (frameElapsed < secondsPerFrame)
				{
					Win32UpdateSound();
					frameElapsed = WallSeconds() - frameStart;
				}
			}
			else
			{
				// TODO: Log missed frame
			}

			frameStart = WallSeconds();
			UpdateWindow(dc, &g.frameBuffer);
		}
	}
	else
	{
		// TODO: logging
	}

	return 0;
}

// Interface to game DLL
internal PlatFuncs_s s_plat = {
    Qi_ReadEntireFile, Qi_WriteEntireFile, Qi_ReleaseFileBuffer, Qi_WallSeconds,
};
const PlatFuncs_s* plat = &s_plat;
#endif
