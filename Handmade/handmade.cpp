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

//TODO(ivan): exclude global variable later
global_variable bool Running;
global_variable BITMAPINFO BitmapInfo;
global_variable	void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void
RenderGradient(int xOffset, int yOffset)
{
	//Fill in the pixels
	int Pitch = BitmapWidth * BytesPerPixel;
	uint8* Row = (uint8*)BitmapMemory;
	for (int y = 0; y < BitmapHeight; ++y)
	{
		uint32* Pixel = (uint32*)Row;
		for (int x = 0; x < BitmapWidth; ++x)
		{
			uint8 Blue = x + xOffset;
			uint8 Green = y + yOffset;
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Pitch;
	}
}

internal void 
Win32ResizeDIBSection(int Width, int Height)
{
	//Before allocation make sure to free buffer
	if (BitmapMemory)
	{
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	//Allocate Memory for the buffer
	int BytesPerPixel = 4;
	int BitmapMemorySize = (BitmapWidth * BitmapHeight) * BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	//TODO(ivan): clear this to black
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT* WindowRect, int X, int Y, int Width, int Height)
{
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;
	StretchDIBits(
		DeviceContext,
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory,
		&BitmapInfo,
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
			RECT clientRect;
			GetClientRect(Window, &clientRect);
			int Width = clientRect.right - clientRect.left;
			int Height = clientRect.bottom - clientRect.top;
			Win32ResizeDIBSection(Width, Height);
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
	windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
				RenderGradient(xOffset, yOffset);
				HDC DeviceContext = GetDC(Window);
				RECT clientRect;
				GetClientRect(Window, &clientRect);
				int WindowWidth = clientRect.right - clientRect.left;
				int WindowHeight = clientRect.bottom - clientRect.top;
				Win32UpdateWindow(DeviceContext, &clientRect, 0, 0, WindowWidth, WindowHeight);
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