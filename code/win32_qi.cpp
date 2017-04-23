#include "basictypes.h"
#include "qi.h"

#include "qi.cpp"
#include "qi_math.cpp"
#include "qi_sound.cpp"

#include <Windows.h>
#include <dsound.h>
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

struct Sound_s
{
	LPDIRECTSOUND       device;
	LPDIRECTSOUNDBUFFER primaryBuffer;
	LPDIRECTSOUNDBUFFER buffer;
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

#define DIRECTSOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECTSOUND_CREATE(impDirectSoundCreate);

double
TimeMillisec()
{
	LARGE_INTEGER curCounter;
	i64           deltaCounter;
	QueryPerformanceCounter(&curCounter);
	deltaCounter = curCounter.QuadPart - g.startupTime.QuadPart;
	return deltaCounter * g.clockConversionFactor;
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
	osb->bmi.bmiHeader.biHeight      = -height;
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
ProcessAnalog(Analog_s* newState, const r32 value)
{
	newState->startEnd.x = newState->startEnd.y = value;
	newState->minMax.x = newState->minMax.y = value;
}

internal void
SampleInput()
{
	Input_s newInput = {};

	for (DWORD controllerIdx = 0; controllerIdx < XUSER_MAX_COUNT; controllerIdx++)
	{
		XINPUT_STATE        controllerState;
		const Controller_s* oldState = &g.inputState.controllers[controllerIdx];
		Controller_s*       newState = &newInput.controllers[controllerIdx];

		if (XInputGetState(controllerIdx, &controllerState) == ERROR_SUCCESS)
		{
			XINPUT_GAMEPAD* xpad = &controllerState.Gamepad;

			newState->analog = true;

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
			ProcessAnalog(&newState->leftTrigger, xpad->bLeftTrigger / 255.0f);
			ProcessAnalog(&newState->rightTrigger, xpad->bRightTrigger / 255.0f);
			ProcessAnalog(&newState->leftStickX, (xpad->sThumbLX + 32768.0f) / 32767.0f - 1.0f);
			ProcessAnalog(&newState->leftStickX, (xpad->sThumbLY + 32768.0f) / 32767.0f - 1.0f);
			ProcessAnalog(&newState->leftStickX, (xpad->sThumbRX + 32768.0f) / 32767.0f - 1.0f);
			ProcessAnalog(&newState->leftStickX, (xpad->sThumbRY + 32768.0f) / 32767.0f - 1.0f);
		}
		else
		{
			memset(newState, 0, sizeof(*newState));
		}
	}

	memcpy(&g.inputState, &newInput, sizeof(Input_s));
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
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		u32  vkCode  = wParam;
		bool wasDown = (lParam & (1 << 30)) != 0;
		bool isDown  = (lParam & (1 << 31)) == 0;

		if (wasDown == isDown)
			break;

		if (vkCode == 'W')
		{
		}
		else if (vkCode == 'D')
		{
		}
		else if (vkCode == 'S')
		{
		}
		else if (vkCode == 'A')
		{
		}
		else if (vkCode == 'Q')
		{
		}
		else if (vkCode == 'E')
		{
		}
		else if (vkCode == VK_UP)
		{
		}
		else if (vkCode == VK_RIGHT)
		{
		}
		else if (vkCode == VK_DOWN)
		{
		}
		else if (vkCode == VK_LEFT)
		{
		}
		else if (vkCode == VK_ESCAPE)
		{
			OutputDebugStringA("ESCAPE: ");
			if (isDown)
				OutputDebugStringA("IsDown ");
			if (wasDown)
				OutputDebugStringA("WasDown");
			OutputDebugStringA("\n");
		}
		else if (vkCode == VK_SPACE)
		{
		}
		else
		{
		}
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

const int kSamplesPerSecond = 48000; // Samples / sec

internal void
UpdateSound(void)
{
	VOID* region1Data;
	DWORD region1Size;
	VOID* region2Data;
	DWORD region2Size;
	DWORD playOffsetBytes;
	DWORD writeOffsetBytes;

	r32        testToneHz   = 261.63; // Cycles / sec
	r32        toneVolume   = 20000;
	u32        latency      = g.soundUpdateBuffer.samplesPerSec / 15;
	u32        latencyBytes = latency * g.soundUpdateBuffer.bytesPerSample;
	static u32 nextByteToLock;
	memset(g.soundUpdateBuffer.bytes, 0, g.soundUpdateBuffer.byteSize);
	DSBCAPS caps;
	caps.dwSize = sizeof(caps);
	g.sound.buffer->GetCaps(&caps);

	if (SUCCEEDED(g.sound.buffer->GetCurrentPosition(&playOffsetBytes, &writeOffsetBytes)))
	{
		u32 byteToLock   = nextByteToLock;
		u32 targetCursor = ((playOffsetBytes + latencyBytes) % caps.dwBufferBytes);
		u32 bytesToWrite;

		if (byteToLock > targetCursor)
		{
			bytesToWrite = caps.dwBufferBytes - byteToLock;
			bytesToWrite += targetCursor;
		}
		else
		{
			bytesToWrite = targetCursor - byteToLock;
		}
		nextByteToLock = ((nextByteToLock + bytesToWrite) % caps.dwBufferBytes);

		GenTone(&g.soundUpdateBuffer, bytesToWrite / g.soundUpdateBuffer.bytesPerSample, testToneHz, toneVolume);

		if (SUCCEEDED(g.sound.buffer->Lock(
		        byteToLock, bytesToWrite, &region1Data, &region1Size, &region2Data, &region2Size, 0)))
		{
			memcpy(region1Data, g.soundUpdateBuffer.bytes, region1Size);
			if (region2Size)
				memcpy(region2Data, g.soundUpdateBuffer.bytes + region1Size, region2Size);

			g.sound.buffer->Unlock(region1Data, region1Size, region2Data, region2Size);
		}
	}
}

internal void
InitSoundBuffer()
{
	DSBCAPS caps;
	caps.dwSize = sizeof(caps);
	g.sound.buffer->GetCaps(&caps);

	SoundBuffer_s buf;
	MakeSoundBuffer(&buf, caps.dwBufferBytes / 4, 2, 48000);
	GenTone(&buf, buf.numSamples, 261.0f, 32000.0f);

	VOID* region1Data;
	DWORD region1Size;
	VOID* region2Data;
	DWORD region2Size;
	g.sound.buffer->Lock(0, caps.dwBufferBytes, &region1Data, &region1Size, &region2Data, &region2Size, 0);
	memcpy(region1Data, buf.bytes, region1Size);
	g.sound.buffer->Unlock(region1Data, region1Size, region2Data, region2Size);

	FreeSoundBuffer(&buf);
}

internal void
InitDirectSound(void)
{
	HMODULE               dsoundLib         = LoadLibraryA("dsound.dll");
	impDirectSoundCreate* DirectSoundCreate = (impDirectSoundCreate*)GetProcAddress(dsoundLib, "DirectSoundCreate");
	if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &g.sound.device, 0)))
	{
		g.sound.device->SetCooperativeLevel(g.wnd, DSSCL_PRIORITY);

		WAVEFORMATEX format    = {};
		format.wFormatTag      = WAVE_FORMAT_PCM;
		format.nChannels       = 2;
		format.wBitsPerSample  = 16;
		format.nSamplesPerSec  = kSamplesPerSecond;
		u16 bytesPerSample     = (format.wBitsPerSample * format.nChannels) / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * bytesPerSample;
		format.nBlockAlign     = bytesPerSample;

		DSBUFFERDESC bufDesc = {};
		bufDesc.dwSize       = sizeof(DSBUFFERDESC);
		bufDesc.dwFlags      = DSBCAPS_PRIMARYBUFFER;

		g.sound.device->CreateSoundBuffer(&bufDesc, &g.sound.primaryBuffer, 0);
		if (g.sound.primaryBuffer)
		{
			if (SUCCEEDED(g.sound.primaryBuffer->SetFormat(&format)))
			{
				bufDesc.dwFlags = 0;
				// 1 seconds worth of audio buffer
				bufDesc.dwBufferBytes = format.nAvgBytesPerSec;
				bufDesc.lpwfxFormat   = &format;
				g.sound.device->CreateSoundBuffer(&bufDesc, &g.sound.buffer, 0);
				OutputDebugStringA("Created Sound Buffer\n");
				InitSoundBuffer();
				g.sound.buffer->SetVolume(0);
			}
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
		bool isPlaying = false;

		InitDirectSound();
		MakeSoundBuffer(&g.soundUpdateBuffer, 48000, 2, 48000);

		HDC dc = GetDC(g.wnd);

		g.gameRunning = true;
		int xOff      = 0;
		int yOff      = 0;
		while (g.gameRunning)
		{
			MSG message;
			while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
			{
				if (message.message == WM_QUIT)
					g.gameRunning = false;
				TranslateMessage(&message);
				DispatchMessage(&message);
			}
			SampleInput();
			UpdateSound();
			if (!isPlaying)
			{
				g.sound.buffer->Play(0, 0, DSBPLAY_LOOPING);
				isPlaying = true;
			}
			DrawGradient(&g.frameBuffer.bitmap, xOff++, yOff++);
			UpdateWindow(dc, &g.frameBuffer);
		}
	}
	else
	{
		// TODO: logging
	}

	return 0;
}
