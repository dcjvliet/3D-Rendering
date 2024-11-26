#include <stdbool.h>
#include <windows.h>
#include <stdint.h>

void fill_area(uint32_t *pixels, HWND hwnd)
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

__declspec(dllexport) void bresenham(int start_x, int start_y, int end_x, int end_y, int thickness, HWND hwnd, uint32_t color)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

    // define our dx, dy, and steps
    int dx = abs(end_x - start_x);
    int dy = abs(end_y - start_y);
    int step_x = (end_x > start_x) ? 1 : -1;
    int step_y = (end_y > start_y) ? 1 : -1;

    int error;
    bool steep;
    // define our error from the line
    if (dx > dy)
    {
        error = dx - dy;
        steep = false;
    }
    else
    {
        error = dy - dx;
        steep = true;
    }

    // define our starting values
    int x = start_x;
    int y = start_y;
    int radius = thickness / 2;

    while (true)
    {
        for (int i = -radius; i <= radius; ++i)
        {
            if (steep)
            {
                pixels[y * width + x + i] = color;
            }
            else
            {
                pixels[(y + i) * width + x] = color;
            }
        }

        if (x == end_x || y == end_y)
        {
            break;
        }

        int two_error = error * 2;
        if (two_error > -dy)
        {
            error -= dy;
            x += step_x;
        }
        if (two_error < dx)
        {
            error += dx;
            y += step_y;
        }
    }
    fill_area(pixels, hwnd);
}