#include <stdbool.h>
#include <stdlib.h>

typedef struct
{
    int *x_values;
    int *y_values;
    int size;
} ArrayPair;

__declspec(dllexport) ArrayPair bresenham(float start_x, float start_y, float end_x, float end_y, int thickness)
{
    float dx, dy, step_x, step_y;
    // define our dx, dy, and steps
    if (end_x > start_x)
    {
        dx = end_x - start_x;
        step_x = 1;
    }
    else
    {
        dx = start_x - end_x;
        step_x = -1;
    }
    if (end_y > start_y)
    {
        dy = end_y - start_y;
        step_y = 1;
    }
    else
    {
        dy = start_y - end_y;
        step_y = -1;
    }

    float error;
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

    // inital size of x and y values
    int capacity = 2;
    int size = 0;

    int *x_values = malloc(capacity * sizeof(int));
    int *y_values = malloc(capacity * sizeof(int));

    while (true)
    {
        for (int i = -radius; i <= radius; ++i)
        {
            // increase memory allocation if necessary
            if (size == capacity)
            {
                capacity *= 2;
                x_values = realloc(x_values, capacity * sizeof(int));
                y_values = realloc(y_values, capacity * sizeof(int));
            }

            if (steep)
            {
                x_values[size] = x + i;
                y_values[size] = y;
                size++;
            }
            else
            {
                x_values[size] = x;
                y_values[size] = y + i;
                size++;
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
    ArrayPair result = {x_values, y_values, size};
    return result;
}