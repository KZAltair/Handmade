/* Developed by Ivan Massyutin*/
/*Copywrite by Ivan Massyutin*/

#include <Windows.h>
#include <stdint.h>
#include <malloc.h>
#include <Xinput.h>
#include <dsound.h>
#include <string>
#include <math.h>



#define internal static //data avaliable only to this file where it is
#define local_persist static 
#define global_variable static 

#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

#include "win32_handmade.h"
#include "handmade.cpp"


//TODO(ivan): exclude global variable later
global_variable bool32 Running;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

struct win32_window_dimension
{
	int Width;
	int Height;
};

//Macro defines function with xinputgetstate
#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef XINPUT_GET_STATE(xinput_get_state);

XINPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);

XINPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable xinput_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name)HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal debug_read_file_result DEBUGPlatformReadEntireFile(char* FileName)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTrancateUint64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents)
			{
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					Result.ContentsSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}
			else
			{
				//Error log
			}
		}
		else
		{
			//Error log
		}
		CloseHandle(FileHandle);
	}
	else
	{
		//Error log
	}
	return Result;
}
internal void DEBUGPlatformFreeFileMemory(void* Memory)
{
	if (Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32 DEBUGPlatformWriteEntireFile(char* FileName, uint32 MemorySize, void* Memory)
{
	bool32 Result = false;

	HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize))
		{
				DWORD BytesWritten;
				if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
				{
					Result = (BytesWritten == MemorySize);
				}
				else
				{
					//Error log
				}
		}
		else
		{
			//Error log
		}
		CloseHandle(FileHandle);
	}
	else
	{
		//Error log
	}
	return Result;
}

internal void
Win32LoadXInput()
{
	//If 1.4 not exists
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	//Load functions from dll
	if (XInputLibrary)
	{
		XInputGetState = (xinput_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (xinput_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
	else
	{
		//TODO(ivan): Log error
	}
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	//Steps:
	//1.Load dll library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	
	if (DSoundLibrary)
	{
		//2. Get DirectSound object as it is COM
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX waveFormat = {};
			waveFormat.wFormatTag = WAVE_FORMAT_PCM;
			waveFormat.nChannels = 2;
			waveFormat.nSamplesPerSec = SamplesPerSecond;
			waveFormat.wBitsPerSample = 16; //CD audio standard
			waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
			waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
			waveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				//3. Create a primary buffer to set the mode
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT Error = PrimaryBuffer->SetFormat(&waveFormat);
					if (SUCCEEDED(Error))
					{
						OutputDebugStringA("Primary buffer was set\n");
					}
					else
					{
						//TODO(ivan): Log error for setting primary buffer
					}
				}
			}
			else
			{
				//TODO(ivan): Log error for cooperative level
			}
			//4. Create a secondary buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &waveFormat;
			HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if (SUCCEEDED(Error))
			{
				OutputDebugStringA("Secondary buffer was set\n");
				//5. Start it player
			}
		}
		else
		{
			//TODO(ivan): log error
		}
	}
}

internal win32_window_dimension 
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT clientRect;
	GetClientRect(Window, &clientRect);
	Result.Width = clientRect.right - clientRect.left;
	Result.Height = clientRect.bottom - clientRect.top;

	return Result;
}

internal void 
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
	//Before allocation make sure to free buffer
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	//Allocate Memory for the buffer
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
	//TODO(ivan): clear this to black
}

internal void
Win32UpdateWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight,
				   int X, int Y, int Width, int Height)
{
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS,
		SRCCOPY
	);

}



internal void Win32ClearSoundBuffer(win32_sound_output* SoundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size, 0)))
	{
		uint8* DestSample = (uint8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		DestSample = (uint8*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void
Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD BytesToLock, DWORD BytesToWrite,
					game_sound_output_buffer* SourceBuffer)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(BytesToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size, 0)))
	{


		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16* DestSample = (int16*)Region1;
		int16* SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16*)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal void Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 isDown)
{
	Assert(NewState->EndedDown != isDown);
	NewState->EndedDown = isDown;
	++NewState->HalfTransitionCount;
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* OldState, DWORD ButtonBit, game_button_state* NewState )
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshhold)
{
	real32 Result = 0;
	if (Value < -DeadZoneThreshhold)
	{
		Result = (real32)Value / 32768.0f;
	}
	else if (Value > DeadZoneThreshhold)
	{
		Result = (real32)Value / 32767.0f;
	}
	return Result;
}

internal void Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
	MSG Message;
	while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
	{
		if (Message.message == WM_QUIT)
		{
			Running = false;
		}
		switch (Message.message)
		{
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = (uint32)Message.wParam;
			bool32 wasDown = ((Message.lParam & (1 << 30)) != 0);
			bool32 isDown = ((Message.lParam & (1 << 31)) == 0);
			if (isDown != wasDown)
			{
				if (VKCode == 'W')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, isDown);
				}
				else if (VKCode == 'A')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, isDown);
				}
				else if (VKCode == 'D')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, isDown);
				}
				else if (VKCode == 'S')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, isDown);
				}
				else if (VKCode == 'Q')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, isDown);
				}
				else if (VKCode == 'E')
				{
					Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, isDown);
				}
				else if (VKCode == VK_UP)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, isDown);
				}
				else if (VKCode == VK_LEFT)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, isDown);
				}
				else if (VKCode == VK_RIGHT)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, isDown);
				}
				else if (VKCode == VK_DOWN)
				{
					Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, isDown);
				}
				else if (VKCode == VK_ESCAPE)
				{
					Running = false;
				}
				else if (VKCode == VK_SPACE)
				{

				}
#if HANDMADE_INTERNAL
				else if (VKCode == 'P')
				{
					if (isDown)
					{
						GlobalPause = !GlobalPause;
					}
				}
#endif
			}
			bool32 AltKeyWasDown = ((Message.lParam & (1 << 29)) != 0);
			if ((VKCode == VK_F4) && AltKeyWasDown)
			{
				Running = false;
			}
		}break;
		default:
		{
			TranslateMessage(&Message);
			DispatchMessageA(&Message);
		}break;
		}
	}
}

internal LRESULT CALLBACK Win32WindowProc(HWND Window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch (msg)
	{
		case WM_SIZE:
		{

		}break;
		case WM_DESTROY:
		{
			Running = false;

		}break;
		case WM_CLOSE:
		{
			Running = false;
		}break;
		case WM_ACTIVATEAPP:
		{

		}break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert("Keyboard messages went not through the game loop!")
		}break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height,
								 X, Y, Width, Height);
			EndPaint(Window, &Paint);

		}break;
		default:
		{
			Result = DefWindowProc(Window, msg, wParam, lParam);
		}break;
	}

	return Result;
}

inline LARGE_INTEGER Win32GetWallClock()
{
	//Measure timing between frames
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);

	return Result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);

	return Result;
}

internal void Win32DebugDrawVerticalLine(win32_offscreen_buffer* BackBuffer, 
											int X, int Top, int Bottom, uint32 Color)
{
	if (Top <= 0)
	{
		Top = 0;
	}
	if (Bottom > BackBuffer->Height)
	{
		Bottom = BackBuffer->Height;
	}
	if ((X >= 0) && (X < BackBuffer->Width))
	{
		uint8* Pixel = (uint8*)BackBuffer->Memory + X * BackBuffer->BytesPerPixel + Top * BackBuffer->Pitch;
		for (int Y = Top; Y < Bottom; ++Y)
		{
			*(uint32*)Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
}

inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer* BackBuffer,
										win32_sound_output* SoundOutput,
										real32 C, int PadX, int Top, int Bottom, DWORD Value, uint32 Color)
{
	real32 XReal32 = (C * (real32)Value);
	int X = PadX + (int)XReal32;
	Win32DebugDrawVerticalLine(BackBuffer, X, Top, Bottom, Color);
}

internal void Win32DebugSyncDisplay(win32_offscreen_buffer* BackBuffer, 
									int MarkerCount, win32_debug_time_marker* Markers,
									int CurrentMarkerIndex,
									win32_sound_output* SoundOutput,
									real32 SecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int LineHeight = 64;


	real32 C = (real32)(BackBuffer->Width - 2*PadX) / (real32)SoundOutput->SecondaryBufferSize;
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex];
		Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0xFFFFFF00;
		DWORD PlayWindowColor = 0xFFFF00FF;
		int Top = PadY;
		int Bottom = PadY + LineHeight;
		if (MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			int FirstTop = Top;
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
			
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;

			Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);

		}
		
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
		Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480 * SoundOutput->BytesPerSample, PlayWindowColor);
		
	}
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	//How many counts per second a CPU is doing
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

	//Set windows scheduler granularity to 1MS so that sleep could be more granular
	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();
	WNDCLASSA windowClass = {};
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc = Win32WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "EngineClass";

	constexpr int MonitorRefreshRateHz = 60;
	constexpr int GameUpdateHz = MonitorRefreshRateHz / 2;
	//1 milliseconds divided by 60HZ = how many seconds elapsed every frame
	real32 TargetSecondPerFrame = 1.0f / (real32)GameUpdateHz; //How much time we expect to go by in seconds for every frame
	

	if (RegisterClassA(&windowClass))
	{
		HWND Window = CreateWindowEx(
		0,
			windowClass.lpszClassName,
			"Engine v0.01",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			hInstance,
			0);
		if (Window)
		{
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample);
			SoundOutput.LatencySampleCount = 3 * (SoundOutput.SamplesPerSecond / GameUpdateHz);
			SoundOutput.SafetyBytes = ((SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz)/3;

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize); //Dsound require a window to be created
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
			
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
			LPVOID BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(16);
			GameMemory.TransientStorageSize = Megabytes(32); //Cast to uint64 if more than 4GB is required

			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{

				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];

				//Measure Performace
				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();

				int DEBUGTimeMarkerIndex = 0;
				win32_debug_time_marker DEBUGTimeMarkers[GameUpdateHz / 2] = {0};
				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;

				//Measure granularity of CPU cycles
				int64 LastCycleCount = __rdtsc();

				Running = true;
				while (Running)
				{

					game_controller_input* OldKeyboardController = &OldInput->Controllers[0];
					game_controller_input* NewKeyboardController = &NewInput->Controllers[0];
					game_controller_input ZeroController = {};
					*NewKeyboardController = ZeroController;
					NewKeyboardController->IsConnected = true;
					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
					{
						NewKeyboardController->Buttons[ButtonIndex].EndedDown =
							OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}
					//All messages including keyboard are coming through this loop
					Win32ProcessPendingMessages(NewKeyboardController);

					if (!GlobalPause)
					{
						//TODO(ivan): Should it be pulled more frequently?

						//Steps to setup:
						//1: Loop through max controllers available - max 4
						//2: Get Controller state

						int MaxControllerCount = XUSER_MAX_COUNT;
						if (MaxControllerCount > (ArrayCount(NewInput->Controllers)) - 1)
						{
							MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
						}
						for (DWORD ControllerIndex = 0; ControllerIndex < (DWORD)MaxControllerCount; ControllerIndex++)
						{
							DWORD OurControllerIndex = ControllerIndex + 1;
							game_controller_input* OldController = &OldInput->Controllers[OurControllerIndex];
							game_controller_input* NewController = &NewInput->Controllers[OurControllerIndex];
							XINPUT_STATE ControllerState;
							if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
							{
								NewController->IsConnected = true;
								//NOTE: This controller is plugged in
								XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

								NewController->IsAnalog = true;
								NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

								//List of all possible buttons at XBOX controller
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
								{
									NewController->StickAverageY = 1.0f;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
								{
									NewController->StickAverageX = -1.0f;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
								{
									NewController->StickAverageX = 1.0f;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
								{
									NewController->StickAverageY = -1.0f;
								}

								real32 ThresholdVlue = 0.5f;
								Win32ProcessXInputDigitalButton((NewController->StickAverageX < -ThresholdVlue) ? 1 : 0,
									&OldController->MoveLeft, 1,
									&NewController->MoveLeft);
								Win32ProcessXInputDigitalButton((NewController->StickAverageX > ThresholdVlue) ? 1 : 0,
									& OldController->MoveRight, 1,
									& NewController->MoveRight);
								Win32ProcessXInputDigitalButton((NewController->StickAverageY < -ThresholdVlue) ? 1 : 0,
									&OldController->MoveUp, 1,
									&NewController->MoveUp);
								Win32ProcessXInputDigitalButton((NewController->StickAverageY > ThresholdVlue) ? 1 : 0,
									& OldController->MoveDown, 1,
									& NewController->MoveDown);

								//bool32 LeftThumb = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
								//bool32 RightThumb = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);

								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionDown,
									XINPUT_GAMEPAD_A, &NewController->ActionDown);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionRight,
									XINPUT_GAMEPAD_B,
									&NewController->ActionRight);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionLeft,
									XINPUT_GAMEPAD_X,
									&NewController->ActionLeft);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionUp,
									XINPUT_GAMEPAD_Y,
									&NewController->ActionUp);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->LeftShoulder,
									XINPUT_GAMEPAD_LEFT_SHOULDER,
									&NewController->LeftShoulder);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->RightShoulder,
									XINPUT_GAMEPAD_RIGHT_SHOULDER,
									&NewController->RightShoulder);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->Start,
									XINPUT_GAMEPAD_START,
									&NewController->Start);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->Back,
									XINPUT_GAMEPAD_BACK,
									&NewController->Back);

							}
							else
							{
								//NOTE: The controller is not avalable
								NewController->IsConnected = false;
							}
						}

						//Do game logic here	
						game_offscreen_buffer Buffer = {};
						Buffer.Memory = GlobalBackBuffer.Memory;
						Buffer.Width = GlobalBackBuffer.Width;
						Buffer.Height = GlobalBackBuffer.Height;
						Buffer.Pitch = GlobalBackBuffer.Pitch;
						Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
						GameUpdateAndRender(&GameMemory, NewInput, &Buffer);

						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

						DWORD PlayCursor;
						DWORD WriteCursor;
						if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
						{
							if (!SoundIsValid)
							{
								SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								SoundIsValid = true;
							}
							DWORD BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample)
								% SoundOutput.SecondaryBufferSize;

							DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample)
								/ GameUpdateHz;
							real32 SecondsLeftUntilFlip = TargetSecondPerFrame - FromBeginToAudioSeconds;
							DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/ TargetSecondPerFrame) *
								(real32)ExpectedSoundBytesPerFrame);

							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
							DWORD SafeWriteCursor = WriteCursor;
							if (SafeWriteCursor < PlayCursor)
							{
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							Assert(SafeWriteCursor >= PlayCursor);
							SafeWriteCursor += SoundOutput.SafetyBytes;
							bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

							DWORD TargetCursor = 0;

							if (AudioCardIsLowLatency)
							{
								TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
							}
							else
							{
								TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
							}
							TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

							DWORD BytesToWrite = 0;
							if (BytesToLock > TargetCursor)
							{
								BytesToWrite = SoundOutput.SecondaryBufferSize - BytesToLock;
								BytesToWrite += TargetCursor;
							}
							else
							{
								BytesToWrite = TargetCursor - BytesToLock;
							}
							game_sound_output_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
							SoundBuffer.Samples = Samples;
							GameGetSoundSamples(&GameMemory, &SoundBuffer);
#if HANDMADE_INTERNAL
							win32_debug_time_marker* Marker = &DEBUGTimeMarkers[DEBUGTimeMarkerIndex];
							Marker->OutputPlayCursor = PlayCursor;
							Marker->OutputWriteCursor = WriteCursor;
							Marker->OutputLocation = BytesToLock;
							Marker->OutputByteCount = BytesToWrite;
							Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

							DWORD UnwrappedWriteCursor = WriteCursor;
							if (UnwrappedWriteCursor < PlayCursor)
							{
								UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
							AudioLatencySeconds = ((real32)AudioLatencyBytes / (real32)SoundOutput.BytesPerSample)
								/ SoundOutput.SamplesPerSecond;
#endif
							Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite, &SoundBuffer);
						}
						else
						{
							SoundIsValid = false;
						}

						//Measure timing between frames
						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

						real32 SecondsElapsedForFrame = WorkSecondsElapsed;
						if (SecondsElapsedForFrame < TargetSecondPerFrame)
						{
							if (SleepIsGranular)
							{
								//Calculate the number of milliseconds left to wait for cpu to get back to work
								DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondPerFrame - SecondsElapsedForFrame));
								if (SleepMS > 0)
								{
									Sleep(SleepMS);
								}

							}
							//real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
							//Assert(TestSecondsElapsedForFrame < TargetSecondPerFrame);
							while (SecondsElapsedForFrame < TargetSecondPerFrame)
							{

								SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
							}
						}
						else
						{
							//Error missed framerate
						}

						LARGE_INTEGER EndCounter = Win32GetWallClock();
						real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
						LastCounter = EndCounter;
						//Wait for MS to Sleep and display a frame
						win32_window_dimension Dimension = Win32GetWindowDimension(Window);
#if HANDMADE_INTERNAL
						Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DEBUGTimeMarkers),
							DEBUGTimeMarkers, DEBUGTimeMarkerIndex - 1, &SoundOutput, TargetSecondPerFrame);
#endif

						Win32UpdateWindow(&GlobalBackBuffer, DeviceContext, Dimension.Width, Dimension.Height,
							0, 0, Dimension.Width, Dimension.Height);


						FlipWallClock = Win32GetWallClock();

#if HANDMADE_INTERNAL
						//This is a debug code
						{
							DWORD PlayCursor;
							DWORD WriteCursor;
							if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
							{
								Assert(DEBUGTimeMarkerIndex < ArrayCount(DEBUGTimeMarkers));
								win32_debug_time_marker* Marker = &DEBUGTimeMarkers[DEBUGTimeMarkerIndex];
								Marker->FlipPlayCursor = PlayCursor;
								Marker->FlipWriteCursor = WriteCursor;

							}
						}
#endif
#if 0
						int32 MSPerFrame = (int32)((1000 * CounterElapsed) / PerfCountFrequency); //Mul by 1000 gives milliseconds
						int32 FPS = (int32)(PerfCountFrequency / CounterElapsed); // Calculate FPS
						int32 MCPF = (int32)(CyclesElapsed / (1000 * 1000));

						OutputDebugStringA(std::to_string(MSPerFrame).c_str());
						OutputDebugStringA(" ms ");
						OutputDebugStringA(std::to_string(FPS).c_str());
						OutputDebugStringA(" FPS ");
						OutputDebugStringA(std::to_string(MCPF).c_str());
						OutputDebugStringA(" MegaCyclesPerFrame");
						OutputDebugStringA("\n");
#endif
						//Swap input
						game_input* Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;



						//Measure cycles of CPU for performance improvements
						int64 EndCycleCount = __rdtsc();
						int64 CyclesElapsed = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;
#if HANDMADE_INTERNAL
						++DEBUGTimeMarkerIndex;
						if (DEBUGTimeMarkerIndex >= ArrayCount(DEBUGTimeMarkers))
						{
							DEBUGTimeMarkerIndex = 0;
						}
#endif
					}
				}
			}
			else
			{
				//TODO(ivan): log error
			}
		}
		else
		{
			//TODO(ivan): log error
		}
	}
	else
	{
		//TODO(ivan): log error
	}

	return 0;
}