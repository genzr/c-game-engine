#include <windows.h>
#include <stdint.h>

#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct win32_window_dimensions
{
	int Width;
	int Height;
};

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};
global_variable win32_offscreen_buffer GlobalBackBuffer;

global_variable bool Running;


internal win32_window_dimensions Win32GetWindowDimension(HWND Window)
{
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

    win32_window_dimensions Result;

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void Win32RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{
	int Pitch = Buffer.Width*4;  // Size of the row in bytes
	uint8* Row = (uint8*)Buffer.Memory;  // Pointer to the first row of the bitmap

    for (int Y = 0; Y < Buffer.Height; Y++)  // For each row
    {
		uint32* Pixel = (uint32*)Row;  // Pointer to the first pixel in the row
        for (int X = 0; X < Buffer.Width; X++) // For each pixel in the row
        {
			uint8 Blue = (X + BlueOffset); // Blue value of the pixel
			uint8 Green = (Y + GreenOffset); // Green value of the pixel

			*Pixel++ = ((Green << 8) | Blue); // Set the pixel value
		}
		Row += Pitch; // Move to the next row
	}
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    Buffer->Info.bmiHeader.biSizeImage = 0;
    Buffer->Info.bmiHeader.biXPelsPerMeter = 0;
    Buffer->Info.bmiHeader.biYPelsPerMeter =0;
    Buffer->Info.bmiHeader.biClrUsed = 0;
    Buffer->Info.bmiHeader.biClrImportant = 0;
    Buffer->BytesPerPixel = 4;

    int BitmapMemorySize = (Buffer->Width* Buffer->Height)*Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer, HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(
		DeviceContext,
		0, 0, Buffer.Width, Buffer.Height,
		0, 0, Width, Height,
		Buffer.Memory,
		&Buffer.Info,
		DIB_RGB_COLORS,
		SRCCOPY
	);
}

LRESULT CALLBACK MainWindowCallback(
    HWND Window,
    UINT Message,
    WPARAM WParam,
    LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_SIZE:
        {
            win32_window_dimensions Dimensions = Win32GetWindowDimension(Window);
            Win32ResizeDIBSection(&GlobalBackBuffer, Dimensions.Width, Dimensions.Height);

            OutputDebugStringA("WM_SIZE\n");
        } break;
        case WM_DESTROY:
        {
            Running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        case WM_CLOSE:
        {
            Running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;

            Win32DisplayBufferInWindow(GlobalBackBuffer, DeviceContext, X, Y, Width, Height);
            EndPaint(Window, &Paint);
            OutputDebugStringA("WM_PAINT\n");
        } break;
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    return Result;
}

int APIENTRY WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, PSTR CommandLine, int ShowCode)
{
    WNDCLASSA WindowClass = {};
    // TODO -  Check if redraws are necessary
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.hInstance = Instance;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);

        if (WindowHandle)
        {
            MSG Msg;
            BOOL MessageResult;

            Running = true;

            uint8 XOffset = 0;
            uint8 YOffset = 0;

            while(Running)
            {
                while (MessageResult = PeekMessage(&Msg, 0, 0, 0, PM_REMOVE))
                {
                    if (MessageResult == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }

                Win32RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

                HDC DeviceContext = GetDC(WindowHandle);
                
                win32_window_dimensions Dimensions = Win32GetWindowDimension(WindowHandle);
                Win32DisplayBufferInWindow(GlobalBackBuffer,  DeviceContext, 0, 0, Dimensions.Width, Dimensions.Height);
                ReleaseDC(WindowHandle, DeviceContext);

                XOffset++;
                YOffset++;
            }
        }
        else
        {
            //TODO - Logging
        }
    } 
    else
    {
        //TODO - Logging
    }

    return 0;
}