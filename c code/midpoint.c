#include <windows.h>
#include <math.h>
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

void horizontal_line(int start_x, int end_x, int y, HWND hwnd, uint32_t color)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

    for (int x = start_x; x < end_x; x++)
    {
        pixels[y * width + x] = color;
    }
    fill_area(pixels, hwnd);
}

void vertical_line(int x, int start_y, int end_y, HWND hwnd, uint32_t color)
{
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

    for (int y = start_y; y < end_y; y++)
    {
        pixels[y * width + x] = color;
    }
    fill_area(pixels, hwnd);
}

__declspec(dllexport) void midpoint_circle(int center_x, int center_y, int radius, int thickness, HWND hwnd, uint32_t color)
{
    thickness++;
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

    if (thickness == 1)
    {
        // starting values for x and y
        int x = 0;
        int y = -radius;
        // we take -r + 0.25 and multiply by 4 to make it an integer
        int determination = -4 * radius + 1;

        // first octant of the circle just draw 8 pixels each time
        while (x < -y)
        {
            // if it's inside the circle
            if (determination < 0)
            {
                determination += 8 * x + 4;
            }
            // if it's outside the circle
            else
            {
                y++;
                determination += 8 * (x + y) + 4;
            }

            pixels[(center_y + y) * width + center_x + x] = color;
            pixels[(center_y + y) * width + center_x - x] = color;
            pixels[(center_y - y) * width + center_x + x] = color;
            pixels[(center_y - y) * width + center_x - x] = color;

            pixels[(center_y + x) * width + center_x + y] = color;
            pixels[(center_y - x) * width + center_x + y] = color;
            pixels[(center_y + x) * width + center_x - y] = color;
            pixels[(center_y - x) * width + center_x - y] = color;
            x++;
        }
        fill_area(pixels, hwnd);
    }
    else
    {
        // starting values for x and y
        int x = 0;
        int y_outer = radius;
        int y_inner = radius - thickness + 1;

        int determination_outer = 4 * y_outer + 1;
        int determination_inner = 4 * y_inner + 1;

        while (y_outer >= x)
        {
            // octant 1
            vertical_line(center_x + x, center_y - y_outer, center_y - y_inner, hwnd, color);
            // octant 2
            horizontal_line(center_x + y_inner, center_x + y_outer, center_y - x, hwnd, color);
            // octant 3
            vertical_line(center_x + x, center_y + y_inner, center_y + y_outer, hwnd, color);
            // octant 4
            horizontal_line(center_x + y_inner, center_x + y_outer, center_y + x, hwnd, color);
            // octant 5
            vertical_line(center_x - x, center_y + y_inner, center_y + y_outer, hwnd, color);
            // octant 6
            horizontal_line(center_x - y_outer, center_x - y_inner, center_y + x, hwnd, color);
            // octant 7
            horizontal_line(center_x - y_outer, center_x - y_inner, center_y - x, hwnd, color);
            // octant 8
            vertical_line(center_x - x, center_y - y_outer, center_y - y_inner, hwnd, color);

            x++;

            if (determination_outer < 0)
            {
                determination_outer += 8 * x + 4;
            }
            else
            {
                y_outer--;
                determination_outer += 8 * (x - y_outer) + 4;
            }

            if (x > radius - thickness + 1)
            {
                y_inner = x;
            }
            else
            {
                if (determination_inner < 0)
                {
                    determination_inner += 8 * x + 4;
                }
                else
                {
                    y_inner--;
                    determination_inner += 8 * (x - y_inner) + 4;
                }
            }
        }
    }
}

__declspec(dllexport) void fill_circle(int center_x, int center_y, int radius, int thickness, HWND hwnd, COLORREF color)
{
    // for some reason a thickness of 3 draws a border of 2, 4 does 3, and so on, so just increase by 1
    thickness++;
    // get window dimensions
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

    radius -= thickness - 1;
    int radius_squared = radius * radius;
    for (int x = -radius; x < radius; x++)
    {
        // find vertical range
        int hh = (int)sqrt(radius_squared - x * x);
        // relative x
        int rx = center_x + x;

        // find y bounds
        int y_start = center_y - hh;
        int y_end = center_y + hh;
        if (y_start < 0)
        {
            y_start = 0;
        }

        for (int y = y_start; y < y_end; y++)
        {
            pixels[y * width + rx] = color;
        }
    }
    fill_area(pixels, hwnd);
}