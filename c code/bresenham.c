#include <stdbool.h>
#include <windows.h>

void draw_pixel(HWND hwnd, int x, int y, COLORREF color)
{
    HDC hdc = GetDC(hwnd);
    SetPixel(hdc, x, y, color);
    ReleaseDC(hwnd, hdc);
}

__declspec(dllexport) void bresenham(float start_x, float start_y, float end_x, float end_y, int thickness, HWND hwnd, COLORREF color)
{
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
                draw_pixel(hwnd, x + i, y, color);
                // x_values[size] = x + i;
                // y_values[size] = y;
                // size++;
            }
            else
            {
                draw_pixel(hwnd, x, y + i, color);
                // x_values[size] = x;
                // y_values[size] = y + i;
                // size++;
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
}