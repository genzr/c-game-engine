#include <windows.h>

#define global_variable static
#define internal static

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;
global_variable void* BitmapMemory;

internal void ResizeDIBSection(int Width, int Height)
{

    if (!BitmapDeviceContext)
    {
        BitmapDeviceContext = CreateCompatibleDC(0);
    }

    if (BitmapHandle)
    {
		DeleteObject(BitmapHandle);
	}

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    BitmapInfo.bmiHeader.biSizeImage = 0;
    BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    BitmapInfo.bmiHeader.biYPelsPerMeter =0;
    BitmapInfo.bmiHeader.biClrUsed = 0;
    BitmapInfo.bmiHeader.biClrImportant = 0;
    
    BitmapHandle =  CreateDIBSection(BitmapDeviceContext,&BitmapInfo,DIB_RGB_COLORS,&BitmapMemory,0,0);    
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

        ResizeDIBSection(Width, Height);
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
        
        PatBlt(
            DeviceContext,
            X,
            Y,
            Width,
            Height,
            WHITENESS
        );

        EndPaint(Window, &Paint);
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
            while(Running)
            {
                MessageResult = GetMessage(&Msg, 0, 0, 0);
                if (MessageResult > 0)
                {
                    TranslateMessage(&Msg);
                    DispatchMessage(&Msg);
                }
                else
                {
                    break;
                }
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