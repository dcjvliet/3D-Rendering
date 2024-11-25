#include <windows.h>

void draw_pixel(HWND hwnd, int x, int y, COLORREF color)
{
    HDC hdc = GetDC(hwnd);
    SetPixel(hdc, x, y, color);
    ReleaseDC(hwnd, hdc);
}

void horizontal_line(int start_x, int end_x, int y, HWND hwnd, COLORREF color)
{
    for (int x = start_x; x <= end_x; x++)
    {
        draw_pixel(hwnd, x, y, color);
    }
}

void vertical_line(int x, int start_y, int end_y, HWND hwnd, COLORREF color)
{
    for (int y = start_y; y <= end_y; y++)
    {
        draw_pixel(hwnd, x, y, color);
    }
}

__declspec(dllexport) void midpoint_circle(int center_x, int center_y, int radius, int thickness, HWND hwnd, COLORREF color)
{
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

            draw_pixel(hwnd, center_x + x, center_y + y, color);
            draw_pixel(hwnd, center_x - x, center_y + y, color);
            draw_pixel(hwnd, center_x + x, center_y - y, color);
            draw_pixel(hwnd, center_x - x, center_y - y, color);
            draw_pixel(hwnd, center_x + y, center_y + x, color);
            draw_pixel(hwnd, center_x - y, center_y + x, color);
            draw_pixel(hwnd, center_x + y, center_y - x, color);
            draw_pixel(hwnd, center_x - y, center_y - x, color);
            x++;
        }
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