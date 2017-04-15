#include "basictypes.h"
#include "qi.h"

#include "qi.cpp"
#include "qi_math.cpp"
#include "qi_sound.cpp"

#include <windows.h>
#include <stdlib.h>
#include <xinput.h>
#include <dsound.h>
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

struct Globals_s
{
	HWND            wnd;
	WindowsBitmap_s frameBuffer;
	bool            gameRunning;
	BITMAPINFO      bmi;
	Input_s         gamePads[XUSER_MAX_COUNT];
	Sound_s         sound;
	LARGE_INTEGER   startupTime;
	double          clockConversionFactor;
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

#define DIRECTSOUND_CREATE(name) \
	HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
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
SampleInput()
{
	for (DWORD controllerIdx = 0; controllerIdx < XUSER_MAX_COUNT; controllerIdx++)
	{
		XINPUT_STATE controllerState;
		Input_s*     input = &g.gamePads[controllerIdx];
		if (XInputGetState(controllerIdx, &controllerState) == ERROR_SUCCESS)
		{
			XINPUT_GAMEPAD* xpad = &controllerState.Gamepad;

			// Buttons
			input->up            = (xpad->wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
			input->right         = (xpad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
			input->down          = (xpad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
			input->left          = (xpad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
			input->aButton       = (xpad->wButtons & XINPUT_GAMEPAD_A) != 0;
			input->bButton       = (xpad->wButtons & XINPUT_GAMEPAD_B) != 0;
			input->xButton       = (xpad->wButtons & XINPUT_GAMEPAD_X) != 0;
			input->yButton       = (xpad->wButtons & XINPUT_GAMEPAD_Y) != 0;
			input->startButton   = (xpad->wButtons & XINPUT_GAMEPAD_START) != 0;
			input->backButton    = (xpad->wButtons & XINPUT_GAMEPAD_BACK) != 0;
			input->leftShoulder  = (xpad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
			input->rightShoulder = (xpad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;

			// Analog
			input->leftTrigger  = xpad->bLeftTrigger / 255.0f;
			input->rightTrigger = xpad->bRightTrigger / 255.0f;
			input->leftStick.x  = xpad->sThumbLX / 32768.0f;
			input->leftStick.y  = xpad->sThumbLY / 32768.0f;
			input->rightStick.x = xpad->sThumbRX / 32768.0f;
			input->rightStick.y = xpad->sThumbRY / 32768.0f;
		}
		else
		{
			memset(input, 0, sizeof(*input));
		}
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

const u32   kBytesPerSample   = sizeof(i16) * 2;
const int   kSamplesPerSecond = 48000; // Samples / sec

internal void
UpdateSound(void)
{
	VOID* region1Data;
	DWORD region1Size;
	VOID* region2Data;
	DWORD region2Size;
	DWORD playOffsetBytes;
	DWORD writeOffsetBytes;

	float testToneHz = 261.63; // Cycles / sec
	float toneVolume = 6000;   // Cycles / sec

	u32   samplesToWrite = kSamplesPerSecond / 4;
	DWORD bytesToWrite   = samplesToWrite * kBytesPerSample;
	if (SUCCEEDED(g.sound.buffer->GetCurrentPosition(&playOffsetBytes, &writeOffsetBytes)))
	{
		g.sound.buffer->Lock(writeOffsetBytes,
		                     bytesToWrite,
		                     &region1Data,
		                     &region1Size,
		                     &region2Data,
		                     &region2Size,
		                     0);

		i16* region1            = (i16*)region1Data;
		u32  region1Samples     = (u32)region1Size / kBytesPerSample;
		i16* region2            = (i16*)region2Data;
		u32  region2Samples     = (u32)region2Size / kBytesPerSample;
		u32  writeOffsetSamples = writeOffsetBytes / kBytesPerSample;

		GenTone(region1,
		        writeOffsetSamples,
		        testToneHz,
		        toneVolume,
		        kSamplesPerSecond,
		        samplesToWrite - region2Samples);
		GenTone(region2,
		        writeOffsetSamples + region1Samples,
		        testToneHz,
		        toneVolume,
		        kSamplesPerSecond,
		        samplesToWrite - region1Samples);

		g.sound.buffer->Unlock(region1Data, region1Size, region2Data, region2Size);
	}
}

internal void
InitDirectSound(void)
{
	HMODULE               dsoundLib = LoadLibraryA("dsound.dll");
	impDirectSoundCreate* DirectSoundCreate
	    = (impDirectSoundCreate*)GetProcAddress(dsoundLib, "DirectSoundCreate");
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
				bufDesc.dwBufferBytes = bytesPerSample * format.nAvgBytesPerSec;
				bufDesc.lpwfxFormat   = &format;
				g.sound.device->CreateSoundBuffer(&bufDesc, &g.sound.buffer, 0);
				OutputDebugStringA("Created Sound Buffer\n");
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
