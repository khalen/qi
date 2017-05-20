#include "basictypes.h"
#include "qi.h"

#include "qi_profile.cpp"
#include "qi.cpp"
#include "qi_math.cpp"
#include "qi_sound.cpp"

#include <Windows.h>
#include <al.h>
#include <alc.h>
#include <stdlib.h>
#include <stdio.h>
#include <Xinput.h>
#include <math.h>

#define internal static

struct WindowsBitmap_s
{
	Bitmap_s   bitmap;
	BITMAPINFO bmi;
};

#define AL_NUM_BUFFERS 3

struct Sound_s
{
	ALCdevice*  alDevice;
	ALCcontext* alContext;
	ALuint      source;
	ALuint      buffers[AL_NUM_BUFFERS];
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
	SoundBuffer_s   soundUpdateBuffer;
	Input_s         inputState;
	Memory_s        memory;
	u32             cursorBuf[16];
	u32             curCursorPos;
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

double
QiPlat_WallSeconds()
{
	LARGE_INTEGER curCounter;
	i64           deltaCounter;
	QueryPerformanceCounter(&curCounter);
	deltaCounter = curCounter.QuadPart - g.startupTime.QuadPart;
	return deltaCounter * g.clockConversionFactor;
}

// Temporary file I/O
void*
Qi_ReadEntireFile(const char* fileName, size_t* fileSize)
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
Qi_WriteEntireFile(const char* fileName, const void* ptr, const size_t size)
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
Qi_ReleaseFileBuffer(void* buffer)
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

void
ProcessKeyboardMessage(Input_s* newInput, MSG* message)
{
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

		Controller_s* kbdController = &newInput->controllers[KBD];
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
		else
		{
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
CheckAlError()
{
	ALCenum error = alGetError();
	if (error != AL_NO_ERROR)
	{
		// TODO: Better logging
		OutputDebugString("OpenAL error!\n");
		__debugbreak();
	}
}

#define CHECK_AL(value) \
	(value);            \
	CheckAlError();

const u32 kAlLatencySamples = QI_SOUND_SAMPLES_PER_SECOND / 1000 * QI_SOUND_REQUESTED_LATENCY_MS;

internal void
InitOpenAL(void)
{
	if (alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT"))
	{
		const ALCchar* devList = CHECK_AL(alcGetString(nullptr, ALC_DEVICE_SPECIFIER));
		OutputDebugString("OpenAL Devices:\n");
		while (devList && *devList != 0)
		{
			OutputDebugString(devList);
			OutputDebugString("\n");
			devList += strlen(devList) + 1;
		}
	}
	g.sound.alDevice  = CHECK_AL(alcOpenDevice(nullptr));

    ALCint params[] = {
        ALC_FREQUENCY, QI_SOUND_SAMPLES_PER_SECOND,
        ALC_REFRESH, 1000 / (QI_SOUND_REQUESTED_LATENCY_MS / 4),
        0, 0
    };
	g.sound.alContext = CHECK_AL(alcCreateContext(g.sound.alDevice, params));
	if (!alcMakeContextCurrent(g.sound.alContext))
		OutputDebugString("Couldn't make OpenAL context current!\n");
	CheckAlError();

	// Create source & buffers
	CHECK_AL(alGenSources(1, &g.sound.source));
	CHECK_AL(alGenBuffers(AL_NUM_BUFFERS, g.sound.buffers));

	// Initialize buffers w/silence and enqueue
	const size_t bufferBytes = kAlLatencySamples * QI_SOUND_BYTES_PER_SAMPLE;
	u16*         tempBuf     = (u16*)malloc(bufferBytes);
	memset(tempBuf, 0, bufferBytes);
	for (u32 bufIdx = 0; bufIdx < AL_NUM_BUFFERS; bufIdx++)
	{
		CHECK_AL(alBufferData(
		    g.sound.buffers[bufIdx], AL_FORMAT_STEREO16, tempBuf, bufferBytes, QI_SOUND_SAMPLES_PER_SECOND));
		CHECK_AL(alSourceQueueBuffers(g.sound.source, 1, &g.sound.buffers[bufIdx]));
	}
}

internal void
Win32UpdateSound()
{
	ALint buffersProcessed = 0;

	CHECK_AL(alGetSourcei(g.sound.source, AL_BUFFERS_PROCESSED, &buffersProcessed));
	while (buffersProcessed)
	{
		ALuint buffer = 0;

		CHECK_AL(alSourceUnqueueBuffers(g.sound.source, 1, &buffer));

		Qis_UpdateSound(&g.soundUpdateBuffer, kAlLatencySamples);

		CHECK_AL(alBufferData(buffer,
		                      AL_FORMAT_STEREO16,
		                      g.soundUpdateBuffer.samples,
		                      kAlLatencySamples * g.soundUpdateBuffer.bytesPerSample,
		                      g.soundUpdateBuffer.samplesPerSec));
		CHECK_AL(alSourceQueueBuffers(g.sound.source, 1, &buffer));
		buffersProcessed--;
	}

	ALint sourceState = 0;
	CHECK_AL(alGetSourcei(g.sound.source, AL_SOURCE_STATE, &sourceState));

	if (sourceState != AL_PLAYING)
	{
		ALint numQueued = 0;
		CHECK_AL(alGetSourcei(g.sound.source, AL_BUFFERS_QUEUED, &numQueued));
		if (numQueued > 0)
		{
			CHECK_AL(alSourcePlay(g.sound.source));
		}
		else
		{
			OutputDebugString("Sound underflow??\n");
		}
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

	g.wnd = CreateWindowExA(0,
	                        "QIClass",
	                        "QI",
	                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
	                        CW_USEDEFAULT,
	                        CW_USEDEFAULT,
	                        CW_USEDEFAULT,
	                        CW_USEDEFAULT,
	                        nullptr,
	                        nullptr,
	                        instance,
	                        nullptr);

	if (g.wnd)
	{
		InitOpenAL();
		timeBeginPeriod(1);

		Qis_MakeSoundBuffer(&g.soundUpdateBuffer, QI_SOUND_SAMPLES_PER_SECOND, 2, QI_SOUND_SAMPLES_PER_SECOND);

		HDC dc = GetDC(g.wnd);

		r64 frameStart = QiPlat_WallSeconds();

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

				switch (message.message)
				{
				case WM_SYSKEYUP:
				case WM_SYSKEYDOWN:
				case WM_KEYUP:
				case WM_KEYDOWN:
				{
					ProcessKeyboardMessage(&newInput, &message);
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
			Qi_GameUpdateAndRender(&g.memory, &g.inputState, &g.frameBuffer.bitmap);
			QiProf_DrawTicks(&g.frameBuffer.bitmap, fpsTicks, countof(fpsTicks), 1024, 10, 50, 0);

			const r64 secondsPerFrame = 1.0 / TARGET_FPS;

			r64 frameElapsed = QiPlat_WallSeconds() - frameStart;
			if (frameElapsed < secondsPerFrame)
			{
                u32 msToSleep = (u32)(1000.0 * (secondsPerFrame - frameElapsed));
                while( msToSleep > QI_SOUND_REQUESTED_LATENCY_MS / 2)
                {
                    Sleep((DWORD)msToSleep);
                    msToSleep -= (QI_SOUND_REQUESTED_LATENCY_MS / 2);
                    Win32UpdateSound();
                }

				if (msToSleep > 0)
					Sleep(msToSleep);

				while (frameElapsed < secondsPerFrame)
                {
					frameElapsed = QiPlat_WallSeconds() - frameStart;
                    Win32UpdateSound();
                }
			}
			else
			{
				// TODO: Log missed frame
			}

			frameStart = QiPlat_WallSeconds();

			UpdateWindow(dc, &g.frameBuffer);
		}
	}
	else
	{
		// TODO: logging
	}

	return 0;
}
