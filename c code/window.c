#include <windows.h>
#include <process.h>

__declspec(dllexport) void create_window();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        SetPixel(hdc, 50, 50, RGB(255, 0, 0));
        EndPaint(hwnd, &ps);

        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

__declspec(dllexport) void message_loop()
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

__declspec(dllexport) void create_window(char winTitle[], int dimensions[])
{
    const char CLASS_NAME[] = "Renderer Window";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        winTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, dimensions[0], dimensions[1],
        NULL,
        NULL,
        wc.hInstance,
        NULL);

    if (hwnd == NULL)
    {
        return;
    }

    ShowWindow(hwnd, SW_SHOW);
}

__declspec(dllexport) HWND get_hwnd(char winTitle[])
{
    HWND hwnd = FindWindow(NULL, winTitle);
    return hwnd;
}

__declspec(dllexport) void draw_pixel(HWND hwnd, int x, int y, COLORREF color)
{
    HDC hdc = GetDC(hwnd);
    SetPixel(hdc, x, y, color);
    ReleaseDC(hwnd, hdc);
}

__declspec(dllexport) void fill_rect(HWND hwnd, int x, int y, int width, int height, COLORREF color)
{
    HDC hdc = GetDC(hwnd);
    HBRUSH brush = CreateSolidBrush(color);
    RECT rect = {x, y, x + width, y + height};
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
    ReleaseDC(hwnd, hdc);
}

__declspec(dllexport) void kill_window(HWND hwnd)
{
    DestroyWindow(hwnd);
}