#include <stdbool.h>
#include <windows.h>

void draw_pixel(HWND hwnd, int x, int y, COLORREF color)
{
    HDC hdc = GetDC(hwnd);
    SetPixel(hdc, x, y, color);
    ReleaseDC(hwnd, hdc);
}

__declspec(dllexport) void midpoint_circle(int center_x, int center_y, int radius, int thickness, HWND hwnd, COLORREF color)
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