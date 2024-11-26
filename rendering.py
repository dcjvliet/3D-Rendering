"""
rendering.py

My humble attempt at creating a graphics engine from scratch
"""
import ctypes
import math
from typing import Union


# load in C code for creating a window and drawing on it
window_lib : ctypes.CDLL = ctypes.CDLL('./dlls/window.dll')
window_lib.create_window.argtypes = [ctypes.POINTER(ctypes.c_char), ctypes.POINTER(ctypes.c_int)]
window_lib.get_hwnd.argtypes = [ctypes.POINTER(ctypes.c_char)]
window_lib.draw_pixel.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_uint32]
window_lib.fill_rect.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
window_lib.kill_window.argtypes = [ctypes.c_void_p]
window_lib.message_loop.argtypes = [ctypes.c_void_p]
window_lib.fill_area.argtypes = [ctypes.POINTER(ctypes.c_uint32), ctypes.c_void_p, ctypes.c_uint]

# ctype for creating the dimensions of the window
DimensionsArray = ctypes.c_int * 2

# loading in the dll for bresenham's algo
bresenham = ctypes.CDLL('./dlls/bresenham.dll')
bresenham.bresenham.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint]
bresenham.wu_line.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint]

# loading in the dll for midpoint circle algo
midpoint_circle = ctypes.CDLL('./dlls/midpoint.dll')
midpoint_circle.midpoint_circle.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint]
midpoint_circle.fill_circle.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint]

def matrix_multiplication(matrix_one : Union[list[float], tuple[float]], matrix_two : Union[list[float], tuple[float]]) -> list[float]:
    """Returns the multiplication of two matrices

    :param matrix_one: The first matrix
    :type matrix_one: Union[list[float], tuple[float]]
    :param matrix_two: The second matrix
    :type matrix_two: Union[list[float], tuple[float]]
    :raises ValueError: If the matrix dimensions are not valid
    :return: The multiplied matrices
    :rtype: list[float]
    """
    # if columns doesn't equal rows
    if len(matrix_one[0]) != len(matrix_two):
        raise ValueError('Incompatible matrix dimensions')
    
    # Initialize the result matrix with zeros
    result = [[0 for _ in range(len(matrix_two[0]))] for _ in range(len(matrix_one))]
    
    # Perform multiplication
    for i in range(len(matrix_one)):  # Rows of the first matrix
        for j in range(len(matrix_two[0])):  # Columns of the second matrix
            for k in range(len(matrix_two)):  # Elements in the shared dimension
                result[i][j] += matrix_one[i][k] * matrix_two[k][j]
    
    return result


class CustomError(Exception):
    """Base class for custom exceptions
    """
    pass


class NoValidHandle(CustomError):
    """Exception raised when a window handle couldn't be found
    """
    pass


class Color:
    """A color that is rgba compatible
    """
    def __init__(self, color : Union[tuple[int], list[int]]) -> None:
        """Initialize the color

        :param color: A tuple containing rgb(a) values
        :type color: Union[tuple[int], list[int]]
        :raises ValueError: If too many arguments are passed in the color parameter
        :raises ValueError: If the rgb(a) values are too large or too small
        """
        # not the right amount of values
        if len(color) != 4 and len(color) != 3:
            raise ValueError('color must be a rgb/rgba color value')
        # values are not in correct range
        if color[0] > 255 or color[0] < 0 or color[1] > 255 or color[1] < 0 or color[2] > 255 or color[2] < 0:
            raise ValueError('r, g, b, and a values must be between 0 and 255')
        # check for alpha value as well
        elif len(color) == 4:
            if color[3] > 255 or color[3] < 0:
                raise ValueError('r, g, b, and a values must be between 0 and 255')
        
        # set values
        if len(color) == 4:
            self.r : int = color[0]
            self.g : int = color[1]
            self.b : int = color[2]
            self.a : int = color[3]
            col = (self.a << 24) | (self.r << 16) | (self.g << 8) | self.b
            self.colorint = ctypes.c_uint32(col)
        else:
            self.r : int = color[0]
            self.g : int = color[1]
            self.b : int = color[2]
            self.a : int = 255
            col = (self.a << 24) | (self.r << 16) | (self.g << 8) | self.b
            self.colorint = ctypes.c_uint32(col)

    @classmethod
    # create the class from a hex code instead
    def from_hex_code(cls, hex_code : str) -> 'Color':
        """Initialize the color from a hex code

        :param hex_code: The color's hex code
        :type hex_code: str
        :raises ValueError: If the length of the hex code doesn't represent a rgb(a) color
        :raises ValueError: If the hex code has incompatible characters
        """
        if len(hex_code) != 8 and len(hex_code) != 6:
            raise ValueError('hex_code must be a rgb/rgba hex code')
        for char in hex_code:
            if char.upper() not in set('0123456789ABCDEF#'):
                raise ValueError('Incompatible character(s) in hex_code')
        
        if len(hex_code) == 8:
            r : int = int(hex_code[:2], 16)
            g : int = int(hex_code[2:4], 16)
            b : int = int(hex_code[4:6], 16)
            a : int = int(hex_code[-2:], 16)

            return cls((r, g, b, a))
        elif len(hex_code) == 9:
            r : int = int(hex_code[1:3], 16)
            g : int = int(hex_code[3:5], 16)
            b : int = int(hex_code[5:7], 16)
            a : int = int(hex_code[-2:], 16)

            return cls((r, g, b, a))
        elif len(hex_code) == 7:
            r : int = int(hex_code[1:3], 16)
            g : int = int(hex_code[3:5], 16)
            b : int = int(hex_code[5:7], 16)
            
            return cls((r, g, b))
        else:
            r : int = int(hex_code[:2], 16)
            g : int = int(hex_code[2:4], 16)
            b : int = int(hex_code[-2:], 16)

            return cls((r, g, b))

    def __str__(self) -> str:
        """Converts color to readable format to be printed

        :return: A readable format of the color
        :rtype: str
        """
        return f'Color({self.color})'
    

class Window:
    """A window created in C that allows for drawing
    """
    def __init__(self, title : str, dimensions : Union[list[int], tuple[int]], background : Color = Color((255, 255, 255))) -> None:
        """Initialize the window

        :param title: The title of the window
        :type title: str
        :param dimensions: The dimensions of the window
        :type dimensions: Union[list[int], tuple[int]]
        :raises NoValidHandle: If a valid window handle couldn't be found
        """
        # convert the window title to a char array
        name_array = (ctypes.c_char * len(title))(*title.encode('utf-8'))
        window_lib.create_window(name_array, DimensionsArray(dimensions[0], dimensions[1]))
        self.title : str = title
        self.width = dimensions[0]
        self.height = dimensions[1]
        self.hwnd = window_lib.get_hwnd(name_array)
        if self.hwnd == 0:
            raise NoValidHandle('No valid window handle could be found')
        self.background = background
        window_lib.fill_rect(self.hwnd, 0, 0, self.width, self.height, self.background.colorint)

    def draw(self, coord : 'Coordinate', color : Color) -> None:
        """Color the pixel at the given coordinate with the given color

        :param coord: The coordinate to be colored
        :type coord: Coordinate
        :param color: The color of the pixel in rgb(a)
        :type color: Color
        """
        window_lib.draw_pixel(self.hwnd, coord.x, coord.y, color.colorint)

    def mainloop(self) -> None:
        """Loop to run the window's message loop in C
        """
        # start the message loop
        window_lib.message_loop(self.hwnd)

    def kill(self) -> None:
        """Method to kill the window
        """
        window_lib.kill_window(self.hwnd)


class Coordinate:
    """A representation of a coordinate in 2D space
    """
    def __init__(self, x : int, y : int) -> None:
        """Initialize the coordinate

        :param x: The x-coordinate of the coordinate
        :type x: int
        :param y: The y-coordinate of the coordinate
        :type y: int
        """
        if not isinstance(x, int) or not isinstance(y, int):
            raise ValueError('x and y must be integers')
        
        self.x : float = x
        self.y : float = y
    
    def distance(self, other : 'Coordinate') -> float:
        """Find the distance between this coordinate and another coordinate

        :param other: The coordinate to find the distance from
        :type other: Coordinate
        :return: The distance between the two coordinates
        :rtype: float
        """
        return math.sqrt((other.x - self.x) ** 2 + (other.y - self.y) ** 2)

    def __str__(self) -> str:
        """Converts coordinate to readable format to be printed

        :return: A readable format of the coordinate
        :rtype: str
        """
        return f'Coordinate({self.x}, {self.y})'


class Line:
    """A line that can be drawn on a window with antialiasing capabilities
    """
    def __init__(self, master : Window, start : Coordinate, end : Coordinate, color : Color = Color((0, 0, 0)), width : int = 1, antialiasing : bool = False) -> None:
        """Initialize the line

        :param master: The master window of the line
        :type master: Window
        :param start: The start coordinate of the line
        :type start: Coordinate
        :param end: The end coordinate of the line
        :type end: Coordinate
        :param color: The color of the line, defaults to Color((0, 0, 0))
        :type color: Color, optional
        :param width: The width (in pixels) of the line, defaults to 1
        :type width: int, optional
        :param antialias: Whether or not the line should be antialiased, defaults to False
        :type antialias: bool, optional
        :raises ValueError: If width is not a positive integer
        """
        if width <= 0 or not isinstance(width, int):
            raise ValueError('width must be a positive integer')
        
        self.master : Window = master
        self.start : Coordinate = start
        self.end : Coordinate = end
        self.color : Color = color
        self.width : int = width
        self.radius : int = width // 2
        self.antialiasing : bool = antialiasing
        try:
            self.slope : float = (self.end.y - self.start.y) / (self.end.x - self.start.x)
        except ZeroDivisionError:
            self.slope : float = None
    
    def display(self) -> None:
        """Display the line on it's master
        """
        # just use the dll (C for the win (like 2x faster than python))
        if not self.antialiasing:
            if self.slope is not None and self.slope != 0:
                bresenham.bresenham(self.start.x, self.start.y, self.end.x, self.end.y, self.width, self.master.hwnd, self.color.colorint)
            # if horizontal or vertical
            elif self.slope == 0:
                window_lib.fill_rect(self.master.hwnd, self.start.x, self.start.y, self.end.x - self.start.x, self.width, self.color.colorint)
            else:
                window_lib.fill_rect(self.master.hwnd, self.start.x, self.start.y, self.width, self.end.y - self.start.y, self.color.colorint)
        else:
            if self.slope is not None and self.slope != 0:
                bresenham.wu_line(self.start.x, self.start.y, self.end.x, self.end.y, self.width, self.master.hwnd, self.color.colorint)
            # if horizontal or vertical
            elif self.slope == 0:
                window_lib.fill_rect(self.master.hwnd, self.start.x, self.start.y, self.end.x - self.start.x, self.width, self.color.colorint)
            else:
                window_lib.fill_rect(self.master.hwnd, self.start.x, self.start.y, self.width, self.end.y - self.start.y, self.color.colorint)


    def undisplay(self) -> None:
        """Clear the line from its master
        """
        if self.slope is not None and self.slope != 0:
            bresenham.bresenham(self.start.x, self.start.y, self.end.x, self.end.y, self.width, self.master.hwnd, self.master.background.colorint)
        # in case it's a vertical/horizontal line
        elif self.slope == 0:
            window_lib.fill_rect(self.master.hwnd, self.start.x, self.start.y, self.end.x - self.start.x, self.width, self.master.background.colorint)
        else:
            window_lib.fill_rect(self.master.hwnd, self.start.x, self.start.y, self.width, self.end.y - self.start.y, self.master.background.colorint)

    def rotate(self, theta : float, radians : bool = True, keep_original : bool = False, center : str = 'center') -> None:
        """Rotate the line by a given angle

        :param theta: The angle to rotate by
        :type theta: float
        :param radians: Whether or not the given angle is in radians, defaults to True
        :type radians: bool, optional
        :param keep_original: Whether or not to keep the original line, defaults to False
        :type keep_original: bool, optional
        :param center: Where to rotate the line about (center, left, right), defaults to 'center'
        :type center: str, optional
        :raises ValueError: If the center of rotation is not either of the endpoints or the center of the line
        """
        # get rid of original if needed
        if not keep_original:
            self.undisplay()
            
        if not radians:
            theta *= math.pi / 180

        rotation_matrix : list = [
            [math.cos(theta), -math.sin(theta)], 
            [math.sin(theta), math.cos(theta)]]
        
        # translate line to origin to be rotated
        if center == 'center':
            translation_x : float = (self.end.x - self.start.x) / 2 + self.start.x
            translation_y : float = (self.end.y - self.start.y) / 2 + self.start.y
        elif center == 'left':
            translation_x : float = self.start.x
            translation_y : float = self.start.y
        elif center == 'right':
            translation_x : float = self.end.x
            translation_y : float = self.end.y
        else:
            raise ValueError(f'{center} is not a recognized rotation point')

        # apply translation
        start_matrix : list = [[self.start.x - translation_x], [self.start.y - translation_y]]
        end_matrix : list = [[self.end.x- translation_x], [self.end.y - translation_y]]

        # do rotation and revert translation
        temp : list = matrix_multiplication(rotation_matrix, start_matrix)
        self.start = Coordinate(int(temp[0][0] + translation_x), int(temp[1][0] + translation_y))

        temp = matrix_multiplication(rotation_matrix, end_matrix)
        self.end = Coordinate(int(temp[0][0] + translation_x), int(temp[1][0]+ translation_y))

        self.display()
        
    def __str__(self) -> str:
        """Converts the line to a readable format

        :return: A readable format of the line
        :rtype: str
        """
        return f'Line({self.start}, {self.end})'


class Rect:
    """A rectangle that can be drawn on a window with antialiasing capabilities
    """
    def __init__(self, master: Window, top_left_corner : Coordinate, width : int, height : int, border_color : Color = Color((0, 0, 0)), borderwidth : int = 1, antialiasing : bool = False, fill : bool = False, fill_color : Color = Color((0, 0, 0))) -> None:
        """Initialize the rectangle

        :param master: The master window of the rectangle
        :type master: Window
        :param top_left_corner: The top left corner of the rectangle
        :type top_left_corner: Coordinate
        :param width: The width of the rectangle
        :type width: int
        :param height: The height of the rectangle
        :type height: int
        :param border_color: The border color of the rectangle, defaults to Color((0, 0, 0))
        :type border_color: Color, optional
        :param borderwidth: The width of the rectangles border, defaults to 1
        :type borderwidth: int, optional
        :param antialiasing: Whether or not the rectangle's edges should be antialiased, defaults to False
        :type antialiasing: bool, optional
        :param fill: Whether or not the rectangle is filled, defaults to False
        :type fill: bool, optional
        :param fill_color: Fill color of the rectangle, defaults to Color((0, 0, 0))
        :type fill_color: Color, optional
        :raises ValueError: If width is not a positive integer
        :raises ValueError: If height is not a positive integer
        :raises ValueError: If borderwidth is not a positive integer
        """
        if width <= 0 or not isinstance(width, int):
            raise ValueError('width must be a postiive integer')
        
        if height <= 0 or not isinstance(height, int):
            raise ValueError('height must be a positive integer')
        
        if borderwidth <= 0 or not isinstance(borderwidth, int):
            raise ValueError('borderwidth must be a positive integer')
        
        self.master : Window = master
        self.width : int = width
        self.height : int = height
        self.border_color : Color = border_color
        self.borderwidth : int = borderwidth
        self.antialiasing : bool = antialiasing
        self.fill : bool = fill
        self.fill_color : Color = fill_color

        # define 4 vertices
        self.top_left : Coordinate = top_left_corner
        self.top_right : Coordinate = Coordinate(top_left_corner.x + width, top_left_corner.y)
        self.bottom_left : Coordinate = Coordinate(top_left_corner.x, top_left_corner.y + height)
        self.bottom_right : Coordinate = Coordinate(top_left_corner.x + width, top_left_corner.y + height)

        # define 4 edges
        self.top_edge : Line = Line(self.master, self.top_left, self.top_right, self.border_color, self.borderwidth, self.antialiasing)
        self.bottom_edge : Line = Line(self.master, self.bottom_left, self.bottom_right, self.border_color, self.borderwidth, self.antialiasing)
        self.left_edge : Line = Line(self.master, self.top_left, self.bottom_left, self.border_color, self.borderwidth, self.antialiasing)
        self.right_edge : Line = Line(self.master, self.top_right, self.bottom_right, self.border_color, self.borderwidth, self.antialiasing)

    def display(self) -> None:
        """Display the rectangle on its master window
        """
        # draw edges
        self.top_edge.display()
        self.bottom_edge.display()
        self.left_edge.display()
        self.right_edge.display()

        # fill in bottom right corner
        window_lib.fill_rect(self.master.hwnd, self.bottom_right.x, self.bottom_right.y, self.borderwidth, self.borderwidth, self.border_color.colorint)

        # fill if needed
        if self.fill:
            window_lib.fill_rect(self.master.hwnd, self.top_left.x + self.borderwidth, self.top_left.y + self.borderwidth, self.width - self.borderwidth, self.height - self.borderwidth, self.fill_color.colorint)

    def change_fill(self) -> None:
        self.fill = not self.fill
        self.display()

    def rotate(self, theta : float, radians : bool = True, keep_original : bool = False) -> None:
        # convert to radians if it's in degrees
        if not radians:
            theta *= math.pi / 180

        # remove original rectangle if needed
        if not keep_original:
            self.top_edge.undisplay()
            self.bottom_edge.undisplay()
            self.left_edge.undisplay()
            self.right_edge.undisplay()
            # also have to remove bottom right corner
            window_lib.fill_rect(self.master.hwnd, self.bottom_right.x, self.bottom_right.y, self.borderwidth, self.borderwidth, self.master.background.colorint)
            
        rotation_matrix : list = [
            [math.cos(theta), -math.sin(theta)], 
            [math.sin(theta), math.cos(theta)]]
        
        # translate vertices to the origin
        translation_x : float = self.top_left.x + self.width // 2
        translation_y : float = self.top_left.y + self.height // 2

        # define our vertices centered around the origin
        top_left_matrix : list = [[self.top_left.x - translation_x], [self.top_left.y - translation_y]]
        top_right_matrix : list = [[self.top_right.x - translation_x], [self.top_right.y - translation_y]]
        bottom_left_matrix : list = [[self.bottom_left.x - translation_x], [self.bottom_left.y - translation_y]]
        bottom_right_matrix : list = [[self.bottom_right.x - translation_x], [self.bottom_right.y - translation_y]]

        # rotate all of the vertices then shift them back
        temp : list = matrix_multiplication(rotation_matrix, top_left_matrix)
        self.top_left = Coordinate(int(temp[0][0] + translation_x), int(temp[1][0] + translation_y))

        temp : list = matrix_multiplication(rotation_matrix, top_right_matrix)
        self.top_right = Coordinate(int(temp[0][0] + translation_x), int(temp[1][0] + translation_y))

        temp : list = matrix_multiplication(rotation_matrix, bottom_left_matrix)
        self.bottom_left = Coordinate(int(temp[0][0] + translation_x), int(temp[1][0] + translation_y))

        temp : list = matrix_multiplication(rotation_matrix, bottom_right_matrix)
        self.bottom_right = Coordinate(int(temp[0][0] + translation_x), int(temp[1][0] + translation_y))

        # redefine edges with new coordinates
        self.top_edge : Line = Line(self.master, self.top_left, self.top_right, self.border_color, self.borderwidth, self.antialiasing)
        self.bottom_edge : Line = Line(self.master, self.bottom_left, self.bottom_right, self.border_color, self.borderwidth, self.antialiasing)
        self.left_edge : Line = Line(self.master, self.top_left, self.bottom_left, self.border_color, self.borderwidth, self.antialiasing)
        self.right_edge : Line = Line(self.master, self.top_right, self.bottom_right, self.border_color, self.borderwidth, self.antialiasing)

        # redisplay rectangle
        self.top_edge.display()
        self.bottom_edge.display()
        self.left_edge.display()
        self.right_edge.display()

    def __str__(self) -> str:
        """Converts the rectangle to a readable format

        :return: A readable format of the rectangle
        :rtype: str
        """
        return f'Rect({self.top_left}, {self.width}, {self.height})'


class Circle:
    """A circle that can be drawn on a window with antialiasing capabilities
    """
    def __init__(self, master : Window, radius : int, center : Coordinate, border_color : Color = Color((0, 0, 0)), borderwidth : int = 1, fill : bool = False, fill_color : Color = Color((0, 0, 0)), antialiasing : bool = False) -> None:
        """Initialize the circle

        :param master: The circle's master window
        :type master: Window
        :param radius: The radius of the circle
        :type radius: int
        :param center: The center coordinate of the circle
        :type center: Coordinate
        :param border_color: The color of the border of the circle, defaults to Color((0, 0, 0))
        :type border_color: Color, optional
        :param borderwidth: The thickness of the circle's border, defaults to 1
        :type borderwidth: int, optional
        :param fill: Whether or not to fill the circle, defaults to False
        :type fill: bool, optional
        :param fill_color: The fill color of the circle, defaults to Color((0, 0, 0))
        :type fill_color: Color, optional
        :param antialiasing: Whether or not the circle should be antialiased, defaults to False
        :type antialiasing: bool, optional
        :raises ValueError: If radius is not a positive integer
        :raises ValueError: If borderwidth is not a positive integer
        """
        if radius <= 0 or not isinstance(radius, int):
            raise ValueError('radius must be a positive integer')

        if borderwidth <= 0 or not isinstance(borderwidth, int):
            raise ValueError('borderwidth must be a positive integer')

        self.master : Window = master
        self.radius : int = radius
        self.center : Coordinate = center
        self.border_color : Color = border_color
        self.borderwidth : int = borderwidth
        self.fill : bool = fill
        self.fill_color : Color = fill_color
        self.antialiasing : bool = antialiasing

    def display(self) -> None:
        """Display the circle on its master
        """
        midpoint_circle.midpoint_circle(self.center.x, self.center.y, self.radius, self.borderwidth, self.master.hwnd, self.border_color.colorint)
        if self.fill:
            midpoint_circle.fill_circle(self.center.x, self.center.y, self.radius, self.borderwidth, self.master.hwnd, self.fill_color.colorint)

    def change_fill(self) -> None:
        """Change the fill of the circle
        """
        self.fill = not self.fill
        self.display()

    def __str__(self) -> str:
        """Converts the circle to a readable format

        :return: A readable format of the circle
        :rtype: str
        """
        return f'Circle({self.radius}, {self.center})'