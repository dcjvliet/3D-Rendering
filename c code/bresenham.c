#include <stdbool.h>
#include <windows.h>
#include <stdint.h>
#include <math.h>

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

void wu_helper(int start_x, int start_y, int end_x, int end_y, HWND hwnd, uint32_t color)
{
    // define bitmap pixel array thingy
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    uint32_t *pixels = (uint32_t *)malloc(width * height * sizeof(uint32_t));

    // swap order if it is drawn from right to left
    if (end_x < start_x)
    {
        int temp_x = end_x;
        end_x = start_x;
        start_x = temp_x;

        int temp_y = end_y;
        end_y = start_y;
        start_y = temp_y;
    }

    int dx = end_x - start_x;
    int dy = end_y - start_y;
    // no need to check for veritcal line because that will be handled in Python
    float slope = (float)dy / dx;
    int x = start_x;
    float actual_y = start_y;

    // get the alpha value using bit-shifting
    uint8_t alpha = (uint8_t)(color >> 24);

    // place pixel above and below the line with correct opacity
    for (int i = 1; i < dx + 0.5; i++)
    {
        // skip the start point since that's a special case
        x++;
        actual_y += slope;
        int y = (int)actual_y;

        // use top for bottom opacity and vice versa
        float top_distance = fabs(y - actual_y);
        float bottom_distance = 1.0f - top_distance;
        // find new alpha values
        uint32_t top_color = color & 0x00FFFFFF;
        top_color |= ((uint32_t)(alpha * bottom_distance + 0.5f) << 24);
        uint32_t bottom_color = color & 0x00FFFFFF;
        bottom_color |= ((uint32_t)(alpha * top_distance + 0.5f) << 24);

        if (y >= 0 && y <= height && x >= 0 && x <= width)
        {
            pixels[y * width + x] = top_color;
            pixels[(y + 1) * width + x] = bottom_color;
        }
    }

    // endpoints now
    uint32_t endpoint_color = color & 0x00FFFFFF;
    endpoint_color |= ((uint32_t)(alpha * 0.5) << 24);
    pixels[start_y * width + start_x] = endpoint_color;
    pixels[end_y * width + end_x] = endpoint_color;
    fill_area(pixels, hwnd);
}

__declspec(dllexport) void wu_line(int start_x, int start_y, int end_x, int end_y, int thickness, HWND hwnd, uint32_t color)
{
    if (thickness == 1)
    {
        wu_helper(start_x, start_y, end_x, end_y, hwnd, color);
    }
    else
    {
        // thickness isn't 1 so we need to draw 3 lines: 1 aliased, 2 antialised on the edges
        if (thickness >= 3)
        {
            int aliased_thickness = thickness - 2;
            bresenham(start_x, start_y, end_x, end_y, aliased_thickness, hwnd, color);

            // increase it if it's odd so that we don't draw over it when we draw anti-aliased lines
            if (aliased_thickness % 2 == 1)
            {
                aliased_thickness++;
            }
            // now draw our two anti-aliased lines. We need to find the new starting points
            // if the slope is greater than 1, the starting point for the bottom anti-aliased line will be (x - aliased-thickness / 2, y)
            // if it's less than 1, the starting point will for the bottom anti-aliased line will be (x, y - aliased_thickness / 2)
            // then just change signs for other anti-aliased line

            float slope = (float)(end_y - start_y) / (end_x - start_x);
            int bottom_start_x, bottom_start_y, bottom_end_x, bottom_end_y, top_start_x, top_start_y, top_end_x, top_end_y;
            if (slope > 1)
            {
                bottom_start_x = start_x - aliased_thickness / 2;
                bottom_start_y = start_y;

                bottom_end_x = end_x - aliased_thickness / 2;
                bottom_end_y = end_y;

                top_start_x = start_x + aliased_thickness / 2;
                top_start_y = start_y;

                top_end_x = end_x + aliased_thickness / 2;
                top_end_y = end_y;
            }
            else
            {
                bottom_start_x = start_x;
                bottom_start_y = start_y - aliased_thickness / 2;

                bottom_end_x = end_x;
                bottom_end_y = end_y - aliased_thickness / 2;

                top_start_x = start_x;
                top_start_y = start_y + aliased_thickness / 2;

                top_end_x = end_x;
                top_end_y = end_y + aliased_thickness / 2;
            }

            // draw the anti-aliased lines now
            wu_helper(bottom_start_x, bottom_start_y, bottom_end_x, bottom_end_y, hwnd, color);
            wu_helper(top_start_x, top_start_y, top_end_x, top_end_y, hwnd, color);
        }
        // thickness is 2: we just need to draw anti-aliased lines
        else
        {
            float slope = (float)(end_y - start_y) / (end_x - start_x);
            int bottom_start_x, bottom_start_y, bottom_end_x, bottom_end_y, top_start_x, top_start_y, top_end_x, top_end_y;
            if (slope > 1)
            {
                bottom_start_x = start_x - 1;
                bottom_start_y = start_y;

                bottom_end_x = end_x - 1;
                bottom_end_y = end_y;

                top_start_x = start_x + 1;
                top_start_y = start_y;

                top_end_x = end_x + 1;
                top_end_y = end_y;
            }
            else
            {
                bottom_start_x = start_x;
                bottom_start_y = start_y - 1;

                bottom_end_x = end_x;
                bottom_end_y = end_y - 1;

                top_start_x = start_x;
                top_start_y = start_y + 1;

                top_end_x = end_x;
                top_end_y = end_y + 1;
            }

            // draw the anti-aliased lines now
            wu_helper(bottom_start_x, bottom_start_y, bottom_end_x, bottom_end_y, hwnd, color);
            wu_helper(top_start_x, top_start_y, top_end_x, top_end_y, hwnd, color);
        }
    }
}