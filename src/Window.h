#ifndef WINDOW_H
#define WINDOW_H

#include "MinWin.h"
#include "3dmath.h"

struct WindowClass {
    const char* name;
    HINSTANCE hInstance;

    WindowClass(const char* name, WNDPROC proc);
    ~WindowClass();
};

struct Window {
    HWND handle;
    size_t width;
    size_t height;
    WindowClass& windowClass;
    //input
    bool keys[0xFE];
    bool old_keys[0xFE];
    bool just_pressed_keys[0xFE];
    bool just_unpressed_keys[0xFE];
    Ivec2 mouse_pos;
    bool locked_mouse = false;
    bool hidden_mouse = false;
    bool prev_hidden_mouse = false;

    bool mouseKeys[3];
    bool old_mouseKeys[3];
    bool just_pressed_mouseKeys[3];
    bool doublePress_mouseKeys[3];
    bool just_unpressed_mouseKeys[3];
    int scroll;

    void (*resizeWindow)(void*) = nullptr;
    void* app = nullptr;

    Window(const char* Title, WindowClass& windowClass, size_t width, size_t height);
    ~Window();

    static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

extern size_t opened_windows;

#endif // WINDOW_H