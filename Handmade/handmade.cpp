/* Developed by Ivan Massyutin*/
/*Copywrite by Ivan Massyutin*/

#include <Windows.h>
#include <stdint.h>

#define internal static //data avaliable only to this file where it is
#define local_persist static 
#define global_variable static 

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;

};

//TODO(ivan): exclude global variable later
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_dimension
{
	int Width;
	int Height;
};

win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	RECT clientRect;
	GetClientRect(Window, &clientRect);
	Result.Width = clientRect.right - clientRect.left;
	Result.Height = clientRect.bottom - clientRect.top;

	return Result;
}

internal void
RenderGradient(win32_offscreen_buffer Buffer, int xOffset, int yOffset)
{
	//Fill in the pixels

	uint8* Row = (uint8*)Buffer.Memory;
	for (int y = 0; y < Buffer.Height; ++y)
	{
		uint32* Pixel = (uint32*)Row;
		for (int x = 0; x < Buffer.Width; ++x)
		{
			uint8 Blue = x + xOffset;
			uint8 Green = y + yOffset;
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Buffer.Pitch;
	}
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
Win32UpdateWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, 
				  win32_offscreen_buffer Buffer, int X, int Y, int Width, int Height)
{
	StretchDIBits(
		DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer.Width, Buffer.Height,
		Buffer.Memory,
		&Buffer.Info,
		DIB_RGB_COLORS,
		SRCCOPY
	);

}

LRESULT CALLBACK Win32WindowProc(HWND Window, UINT msg, WPARAM wParam, LPARAM lParam)
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
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32UpdateWindow(DeviceContext, Dimension.Width, Dimension.Height, 
								GlobalBackBuffer, X, Y, Width, Height);
			EndPaint(Window, &Paint);

		}break;
		default:
		{
			Result = DefWindowProc(Window, msg, wParam, lParam);
		}break;
	}

	return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS windowClass = {};
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = Win32WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = "EngineClass";
	
	if (RegisterClass(&windowClass))
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
			int xOffset = 0;
			int yOffset = 0;
			Running = true;
			while(Running)
			{
				MSG Message;
				while (PeekMessage(&Message,0,0, 0, PM_REMOVE))
				{
					if (Message.message == WM_QUIT)
					{
						Running = false;
					}
					TranslateMessage(&Message);
					DispatchMessageA(&Message);
				}
				RenderGradient(GlobalBackBuffer, xOffset, yOffset);

				HDC DeviceContext = GetDC(Window);
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);;
				Win32UpdateWindow(DeviceContext, Dimension.Width, Dimension.Height, 
									GlobalBackBuffer, 0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);
				++xOffset;
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