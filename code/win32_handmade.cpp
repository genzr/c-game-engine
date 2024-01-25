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


global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int WindowWidth;
global_variable int WindowHeight;

internal void RenderWeirdGradient(int BlueOffset, int GreenOffset)
{
	int Pitch = BitmapWidth*4;  // Size of the row in bytes
	uint8* Row = (uint8*)BitmapMemory;  // Pointer to the first row of the bitmap

    for (int Y = 0; Y < BitmapHeight; Y++)  // For each row
    {
		uint32* Pixel = (uint32*)Row;  // Pointer to the first pixel in the row
        for (int X = 0; X < BitmapWidth; X++) // For each pixel in the row
        {
			uint8 Blue = (X + BlueOffset); // Blue value of the pixel
			uint8 Green = (Y + GreenOffset); // Green value of the pixel

			*Pixel++ = ((Green << 8) | Blue); // Set the pixel value
		}
		Row += Pitch; // Move to the next row
	}
}

internal void Win32ResizeDIBSection(int Width, int Height)
{
    
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
    BitmapInfo.bmiHeader.biSizeImage = 0;
    BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    BitmapInfo.bmiHeader.biYPelsPerMeter =0;
    BitmapInfo.bmiHeader.biClrUsed = 0;
    BitmapInfo.bmiHeader.biClrImportant = 0;

    int BytesPerPixel = 4;
    int BitmapMemorySize = (BitmapWidth*BitmapHeight)*BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    
}

internal void Win32UpdateWindow(RECT WindowRect, HDC DeviceContext, int X, int Y, int Width, int Height)
{
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
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);

        int Height = ClientRect.bottom - ClientRect.top;
        int Width = ClientRect.right - ClientRect.left;

        Win32ResizeDIBSection(Width, Height);

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
        /*
        PAINTSTRUCT Paint;

        HDC DeviceContext = BeginPaint(Window, &Paint);

        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;
        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        
        RECT ClientRect;
        GetClientRect(Window, &ClientRect);

        Win32UpdateWindow(&ClientRect, DeviceContext, X, Y, Width, Height);

        EndPaint(Window, &Paint);
        */
    }
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
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
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

                RenderWeirdGradient(XOffset, YOffset);

                HDC DeviceContext = GetDC(WindowHandle);
                RECT ClientRect;
                GetClientRect(WindowHandle, &ClientRect);
                WindowWidth = ClientRect.right - ClientRect.left;
                WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(ClientRect, DeviceContext, 0, 0, WindowWidth, WindowHeight);
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