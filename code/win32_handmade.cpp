#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

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

global_variable uint8 XOffset = 0;
global_variable uint8 YOffset = 0;

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

// We need to define the XInputGetState function ourselves because the XInput library is not included in the Windows SDK by default
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_


#define X_INPUT_SET_STATE(name) DWORD WINAPI name(WORD dwUserIndex,XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
   return ERROR_DEVICE_NOT_CONNECTED; 
}
global_variable x_input_set_state* XInputSetState_;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32InitDSound()
{
    //Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary)
    {
        direct_sound_create* DirectSoundCreate_ = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        if(DirectSoundCreate_ != nullptr)
        {
            //Get a DirectSound object 
            LPDIRECTSOUND DirectSound;
            if(DirectSoundCreate_(0, &DirectSound, 0) == DS_OK) 
            {
                OutputDebugStringA("DirectSound created successfully\n");
                // "Create" a primary buffer
                // "Create" a secondary buffer
                // Start playing
            }
        }
        else 
        {
            //TODO - Logging
        }
    }


}
internal void Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
    }
}

internal win32_window_dimensions Win32GetWindowDimension(HWND Window)
{
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

    win32_window_dimensions Result;

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void Win32RenderWeirdGradient(win32_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	int Pitch = Buffer->Width*4;  // Size of the row in bytes
	uint8* Row = (uint8*)Buffer->Memory;  // Pointer to the first row of the bitmap

    for (int Y = 0; Y < Buffer->Height; Y++)  // For each row
    {
		uint32* Pixel = (uint32*)Row;  // Pointer to the first pixel in the row
        for (int X = 0; X < Buffer->Width; X++) // For each pixel in the row
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

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer, HDC DeviceContext, int X, int Y, int Width, int Height)
{
    StretchDIBits(
		DeviceContext,
		0, 0, Width, Height,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
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

            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, X, Y, Width, Height);
            EndPaint(Window, &Paint);
            OutputDebugStringA("WM_PAINT\n");
        } break;
        case WM_KEYUP:
        {
        }break;
        case WM_KEYDOWN:
        {
            uint32 VKeyCode = WParam;
            uint32 IsDown = (LParam & (1 << 31)) == 0;
            uint32 WasDown = (LParam & (1 << 30)) != 0;

            //Only register if key was not already down
           if(WasDown != 1 && IsDown)
            {
                //TODO - Finish adding support for other keys
                if(VKeyCode == VK_SPACE)
                {
                    XOffset += 10;
                    YOffset += 10;
                }
                else if(VKeyCode == VK_ESCAPE)
                {
                    Running = false;
                }
                else if(VKeyCode == VK_UP)
                {
                    YOffset += 10;
                }
                else if(VKeyCode == VK_DOWN)
                {
                    YOffset -= 10;
                }
                else if(VKeyCode == VK_LEFT)
                {
                    XOffset -= 10;
                }
                else if(VKeyCode == VK_RIGHT)
                {
                    XOffset += 10;
                }
                else if (VKeyCode == 'A')
                {
                    OutputDebugStringA("A\n");
                }
                else if (VKeyCode == 'S')
                {
                    OutputDebugStringA("S\n");
                }
                else if (VKeyCode == 'D')
                {
                    OutputDebugStringA("D\n");
                }
                else if (VKeyCode == 'F')
                {
                    OutputDebugStringA("F\n");
                }
            }
        }break;
        case WM_SYSKEYUP:
        {
        }break;
        case WM_SYSKEYDOWN:
        {
            //Handle alt + f4
            uint32 VKeyCode = WParam;
            uint32_t AltKeyWasDown = (LParam & (1 << 29));
            if(VKeyCode == VK_F4 && AltKeyWasDown)
            {
                Running = false;
            }
        }break;
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

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
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

            HDC DeviceContext = GetDC(WindowHandle);

            Win32InitDSound();

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

                for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
                {
                    XINPUT_STATE ControllerState;
                    DWORD XInputState = XInputGetState(0, &ControllerState);

                    if(XInputState == ERROR_SUCCESS) // ERROR_SUCCESS actually means the function succeeeds...
                    {
                        XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

                        WORD Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                        WORD Down = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                        WORD Left = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                        WORD Right = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                        WORD Start = Pad->wButtons & XINPUT_GAMEPAD_START;
                        WORD Back = Pad->wButtons & XINPUT_GAMEPAD_BACK;
                        WORD LeftShoulder = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                        WORD RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                        WORD AButton = Pad->wButtons & XINPUT_GAMEPAD_A;
                        WORD BButton = Pad->wButtons & XINPUT_GAMEPAD_B;
                        WORD XButton = Pad->wButtons & XINPUT_GAMEPAD_X;
                        WORD YButton = Pad->wButtons & XINPUT_GAMEPAD_Y;

                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        if(AButton)
                        {
                            YOffset += 2;
                        }
                    }
                } 

                Win32RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

                win32_window_dimensions Dimensions = Win32GetWindowDimension(WindowHandle);
                Win32DisplayBufferInWindow(&GlobalBackBuffer,  DeviceContext, 0, 0, Dimensions.Width, Dimensions.Height);
                ReleaseDC(WindowHandle, DeviceContext);
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