#include "Window.h"

size_t opened_windows = 0;

WindowClass::WindowClass(const char* name, WNDPROC proc) : name(name) {
    hInstance = GetModuleHandleA(NULL);
    WNDCLASSEXA windowClass = {0};
    windowClass.cbSize = sizeof(WNDCLASSEXA);
    windowClass.style = 0;
    windowClass.lpfnWndProc = proc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInstance;
    windowClass.hIcon = NULL;
    windowClass.hCursor = NULL;
    windowClass.hbrBackground = NULL;
    windowClass.lpszMenuName = name;
    windowClass.lpszClassName = name;
    windowClass.hIconSm = NULL;
    RegisterClassExA(&windowClass);
}

WindowClass::~WindowClass() {
    UnregisterClassA(name, hInstance);
}

Window::Window(const char* Title, WindowClass& windowClass, size_t width, size_t height)
    : windowClass(windowClass), width(width), height(height) {
    RECT wr;
    wr.left = 100;
    wr.right = width + wr.left;
    wr.top = 100;
    wr.bottom = height + wr.top;
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, false);

    handle = CreateWindowExA(
        0,
        windowClass.name,
        Title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, windowClass.hInstance, this
    );

    ShowWindow(handle, SW_SHOW);

    memset(keys,0,sizeof(keys));
    memset(old_keys,0,sizeof(old_keys));
    memset(just_pressed_keys,0,sizeof(just_pressed_keys));
    memset(just_unpressed_keys,0,sizeof(just_unpressed_keys));
    memset(mouseKeys,0,sizeof(mouseKeys));
    memset(old_mouseKeys,0,sizeof(old_mouseKeys));
    memset(just_pressed_mouseKeys,0,sizeof(just_pressed_mouseKeys));
    memset(doublePress_mouseKeys,0,sizeof(doublePress_mouseKeys));
    memset(just_unpressed_mouseKeys,0,sizeof(just_unpressed_mouseKeys));
    scroll = 0;
    mouse_pos = {0,0};
    locked_mouse = false;
    hidden_mouse = false;
    prev_hidden_mouse = false;

    opened_windows += 1;
}

Window::~Window() {
    if (handle != nullptr) {
        DestroyWindow(handle);
    }

    opened_windows -= 1;
}

LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        const CREATESTRUCTA* const pCreate = reinterpret_cast<CREATESTRUCTA*>(lParam);
        Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtrA(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
        SetWindowLongPtrA(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));

        return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));
    return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg){
        case WM_CLOSE: delete this; break;
        case WM_KEYDOWN:
        {
            keys[wParam] = 1;

            if (old_keys[wParam] == 0) {
                just_pressed_keys[wParam] = 1;
            }
            old_keys[wParam] = 1;

            break;
        }

        case WM_KEYUP:
        {
            keys[wParam] = 0;

            if (old_keys[wParam] == 1) {
                just_unpressed_keys[wParam] = 1;
            }
            old_keys[wParam] = 0;

            break;
        }
        
        case WM_SIZE:
        {
            RECT rect = {};
            GetClientRect(hWnd, &rect);

            width = rect.right - rect.left;
            height = rect.bottom - rect.top;
            if(resizeWindow != nullptr && app != nullptr) resizeWindow(app);
            break;
        }
        case WM_LBUTTONDOWN:
        {
            mouseKeys[0] = 1;
            if(old_mouseKeys[0] == 0){
                just_pressed_mouseKeys[0] = 1;
            }
            old_mouseKeys[0] = 1;
            break;
        }

        case WM_LBUTTONUP:
        {
            mouseKeys[0] = 0;
            if(old_mouseKeys[0] == 1){
                just_unpressed_mouseKeys[0] = 0;
            }
            old_mouseKeys[0] = 0;
            break;
        }

        case WM_LBUTTONDBLCLK:
        {
            doublePress_mouseKeys[0] = 1;
            break;
        }

        case WM_RBUTTONDOWN:
        {
            mouseKeys[2] = 1;
            if(old_mouseKeys[2] == 0){
                just_pressed_mouseKeys[2] = 1;
            }
            old_mouseKeys[2] = 1;
            break;
        }

        case WM_RBUTTONUP:
        {
            mouseKeys[2] = 0;
            if(old_mouseKeys[2] == 1){
                just_pressed_mouseKeys[2] = 0;
            }
            old_mouseKeys[2] = 0;
            break;
        }

        case WM_RBUTTONDBLCLK:
        {
            doublePress_mouseKeys[2] = 1;
            break;
        }

        case WM_MBUTTONDOWN:
        {
            mouseKeys[1] = 1;
            if(old_mouseKeys[1] == 0){
                just_pressed_mouseKeys[1] = 1;
            }
            old_mouseKeys[1] = 1;
            break;
        }

        case WM_MBUTTONUP:
        {
            mouseKeys[1] = 0;
            if(old_mouseKeys[1] == 1){
                just_pressed_mouseKeys[1] = 0;
            }
            old_mouseKeys[1] = 0;
            break;
        }

        case WM_MBUTTONDBLCLK:
        {
            doublePress_mouseKeys[1] = 1;
            break;
        }

        case WM_MOUSEWHEEL:{

            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);

            scroll = zDelta;

            break;
        }

        case WM_MOUSEMOVE:
        {
            mouse_pos = {LOWORD(lParam), HIWORD(lParam)};

            break;
        }

        case WM_SETCURSOR:
        {
            if (LOWORD(lParam) == HTCLIENT) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return TRUE;
            }
            break;
        }
    }
    return DefWindowProcA(hWnd, msg, wParam, lParam);
}