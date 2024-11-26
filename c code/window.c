#include <windows.h>
#include <process.h>
#include <stdint.h>

__declspec(dllexport) void create_window();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

__declspec(dllexport) void message_loop(HWND hwnd)
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

__declspec(dllexport) void fill_area(uint32_t *pixels, HWND hwnd)
{
    // get window dimensions
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP oldMap = (HBITMAP)SelectObject(memDC, hBitmap);

    memcpy(pBits, pixels, width * height * sizeof(uint32_t));

    // paint bitmap onto window
    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    AlphaBlend(hdc, 0, 0, width, height, memDC, 0, 0, width, height, blend);

    // cleanup
    SelectObject(memDC, oldMap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
}

__declspec(dllexport) void draw_pixel(HWND hwnd, int x, int y, uint32_t color)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));
    pixels[y * width + x] = color;
    fill_area(pixels, hwnd);
}

__declspec(dllexport) void fill_rect(HWND hwnd, int x, int y, int width, int height, uint32_t color)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int screen_width = rect.right - rect.left;
    int screen_height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(screen_width * screen_height * sizeof(uint32_t));

    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            int pixel_x = x + col;
            int pixel_y = y + row;
            if (pixel_x >= 0 && pixel_x <= screen_width && pixel_y >= 0 && pixel_y <= screen_height)
            {
                pixels[pixel_y * screen_width + pixel_x] = color;
            }
        }
    }
    fill_area(pixels, hwnd);
}

__declspec(dllexport) void kill_window(HWND hwnd)
{
    DestroyWindow(hwnd);
}