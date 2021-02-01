#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define fperror(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define fprintinfo(index, fmt, ...) printf("info:%i:"fmt, index, ##__VA_ARGS__)

COORD CreateCoord(SHORT X, SHORT Y)
{
    COORD Coord = {X, Y};
    return Coord;
}

WINBOOL GetConsoleWindowRect(HANDLE hConsoleOutput, PSMALL_RECT lpSmallRect)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
        return FALSE;

    *lpSmallRect = csbi.srWindow;
    return TRUE;
}

WINBOOL GetConsoleCursorPosition(HANDLE hConsoleOutput, PCOORD lpConsoleCursorPosition)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
        return FALSE;

    *lpConsoleCursorPosition = csbi.dwCursorPosition;
    return TRUE;
}

WINBOOL EnableConsoleResizing(HWND hConsoleWindow, WINBOOL bEnable)
{
    LONG lConsoleWindowStyle = GetWindowLongPtrA(hConsoleWindow, GWL_STYLE);
    if (lConsoleWindowStyle == 0)
        return FALSE;

    if (bEnable)
        lConsoleWindowStyle = lConsoleWindowStyle | WS_SIZEBOX | WS_MAXIMIZEBOX;
    else
        lConsoleWindowStyle = lConsoleWindowStyle & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX;

    if (SetWindowLongPtrA(hConsoleWindow, GWL_STYLE, lConsoleWindowStyle) == 0)
        return FALSE;

    return TRUE;
}

WINBOOL ActuallyMoveWindow(HWND hWnd, int x_offset, int y_offset, WINBOOL bRepaint/*idkwhatitmeanssoimalwaysjustgonnasetittoTRUE:D*/)
{
    RECT WindowRect;
    if (!GetWindowRect(hWnd, &WindowRect))
        return FALSE;

    if (!MoveWindow(hWnd, WindowRect.left + x_offset, WindowRect.top + y_offset, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, bRepaint))
        return FALSE;

    return TRUE;
}

WINBOOL ResizeWindow(HWND hWnd, int width, int height, WINBOOL bRepaint)
{
    RECT WindowRect;
    if (!GetWindowRect(hWnd, &WindowRect))
        return FALSE;

    if (!MoveWindow(hWnd, WindowRect.left, WindowRect.top, width, height, bRepaint))
        return FALSE;

    return TRUE;
}

// this is code gore isnt it
int alphabet_caseinsensitive_strcmp(const char *x, const char *y)
{
    for (; *x && *y && (*x >= 'A' && *x <= 'Z' ? *x+32 : *x) == (*y >= 'A' && *y <= 'Z' ? *y+32 : *y); ++x, ++y);
    return !(*x || *y);
}

// im sorry for doing this, code readers...
int get_two_long_arguments(int *index, int argc, char **argv, unsigned base, long long int *x, long long int *y, const char *argerror, const char *xerr, const char *yerr)
{
    int i = *index;
    if (argc-i-1 < 2)
    {
        // "Option \'setcursorpos\' needs 2 additional argument, but got %i\n"
        printf(argerror, i, argc-i-1);
        return 1;
    }
    char *x_str = argv[i+1];
    char *y_str = argv[i+1];
    int xv = strtoll(x_str, &x_str, base), yv = strtoll(y_str, &y_str, base);
    if (*x_str != 0)
    {
        // "Invalid integer \"%s\" for x on option \"setcursorpos\"\n"
        printf(xerr, i+1, argv[i+1]);
        return 1;
    }
    if (*y_str != 0)
    {
        // "Invalid integer \"%s\" for y on option \"setcursorpos\"\n"
        printf(yerr, i+2, argv[i+2]);
        return 1;
    }
    *x = xv;
    *y = yv;
    *index += 2;
    return 0;
}

int get_two_int_arguments(int *index, int argc, char **argv, unsigned base, int *x, int *y, const char *argerror, const char *xerr, const char *yerr)
{
    long long int xl, yl;
    if (get_two_long_arguments(index, argc, argv, base, &xl, &yl, argerror, xerr, yerr) != 0)
        return 1;

    *x = xl;
    *y = yl;
    return 0;
}



int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("Changer options(what does that even mean lol):\n"
               "    -nb (number base) <base>\n"
               "        Change the base number system (or whatever) of the program, any arguments with type integer in them will will receive a number with the selected base (the default base is base 10) (this function always uses base 10)\n\n"
               "Options:\n"
               "    setcursorposition <x:integer> <y:integer>\n"
               "        Set the console cursor position to (x, y)\n\n"
               "    movecursor <x_offset:integer> <y_offset:integer>\n"
               "        Moves the console cursor position by (x_offset, y_offset)\n\n"
               "    setresizable <true|false>\n"
               "        Changes the window style of the console (removing WS_SIZEBOX and WS_MAXIMIZEBOX) to disable console window resizing (the argument for this option is case insensitive)\n\n"
               "    setwindowpos <x> <y>\n"
               "        Set the position of the console window to (x, y) in pixel coordinates\n\n"
               "    movewindow <x_offset> <y_offset>\n"
               "        Moves the console window position by (x_offset, y_offset) in pixel coordinates\n\n"
               /*"    resizewindow <width> <height>\n"
               "        Resizes the console window size to (width, height) in character cells\n\n"*/
               "    resizewindowpixel <width> <height>\n"
               "        Resizes the console window to size (width, heihgt) in pixels\n\n"
               "Additional Information:\n"
               "    The error format will be like this:\n"
               "        error:[argument position]: [error] with [\"errno\" or \"Window System Error Code\"] [error code] [what the error means if its not a windows system error code]\n\n");

        return 0;
    }
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    HWND hConsoleWindow = GetConsoleWindow();
    if (hConsoleOutput == NULL)
    {
        fperror("Failed to get the console screen buffer handle thing with Windows System Error Code %lu\n", GetLastError());
        return 1;
    }
    if (hConsoleWindow == NULL)
    {
        fperror("Failed to get the console window handle with Windows System Error Code %lu\n", GetLastError());
        return 1;
    }
    unsigned base = 10;
    for (int i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            // Why did i do this?
            if (argv[i][1] == 'n' && argv[i][2] == 'b')
            {
                if (argc-i-1 < 1)
                {
                    printf("Option \'-nb\' needs 1 additional argument, but got %i\n", argc-i-1);
                    return 1;
                }
                char *newbase_str = argv[++i];
                unsigned temp_base = (unsigned)strtoul(newbase_str, &newbase_str, 10);
                if (*newbase_str != 0)
                {
                    printf("Invalid integer \"%s\" for option \'-nb\'\n", argv[i]);
                    return 1;
                }
                if (temp_base > 16)
                {
                    printf("Invalid base \"%u\", the maximum base for this program is 16\n", base);
                    return 1;
                }
                base = temp_base;
            }
        }
        else if (!strcmp("setcursorpos", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y,
                "error:%i:Option \'setcursorpos\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x on option \"setcursorpos\"\n",
                "error:%i:Invalid integer \"%s\" for y on option \"setcursorpos\"\n") != 0)
                return 1;

            if (!SetConsoleCursorPosition(hConsoleOutput, CreateCoord(x, y)))
            {
                fperror("Failed to set the console cursor position to (%u, %u) with Windows System Error Code %lu\n", x, y, GetLastError());
                return 1;
            }
        }
        else if (!strcmp("movecursor", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y,
                "error:%i:Option \'movecursor\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x_offset on option \"movecursor\"\n",
                "error:%i:Invalid integer \"%s\" for y_offset on option \"movecursor\"\n") != 0)
                return 1;

            COORD console_cursor_position;
            if (!GetConsoleCursorPosition(hConsoleOutput, &console_cursor_position))
            {
                printf("Failed to get the console's cursor position on option \"movecursor\" with Windows System Error Code %lu\n", GetLastError());
                return 1;
            }
            if (!SetConsoleCursorPosition(hConsoleOutput, CreateCoord(console_cursor_position.X + x, console_cursor_position.Y + y)))
            {
                fperror("Failed to move the console cursor position by (%u, %u) with Windows System Error Code %lu\n", x, y, GetLastError());
                return 1;
            }
        }
        else if (!strcmp("setresizable", argv[i]))
        {
            if (argc-i-1 < 1)
            {
                printf("Option \'setresizable\' needs 1 additional argument, but got %i\n", argc-i-1);
                return 1;
            }
            const char *arg = argv[i+1];
            WINBOOL value;
            if (alphabet_caseinsensitive_strcmp(arg, "true"))
            {
                value = TRUE;
            }
            else if (alphabet_caseinsensitive_strcmp(arg, "false"))
            {
                value = FALSE;
            }
            else
            {
                printf("Invalid boolean value \"%s\" on option setresizable", arg);
                return 1;
            }
            if (!EnableConsoleResizing(hConsoleWindow, value))
            {
                fperror("Failed to enable console resizing with Windows System Error Code %lu\n", GetLastError());
                return 1;
            }
            ++i;
        }
        else if (!strcmp("setwindowpos", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y,
                "error:%i:Option \'setcursorpos\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x on option \"setcursorpos\"\n",
                "error:%i:Invalid integer \"%s\" for y on option \"setcursorpos\"\n") != 0)
                return 1;

            RECT WindowRect;
            if (!GetWindowRect(hConsoleWindow, &WindowRect))
            {
                fperror("error:%i:Failed to get the console window rect with Windows System Error Code %lu\n", i, GetLastError());
                return 1;
            }
            if (!MoveWindow(hConsoleWindow, x, y, WindowRect.right-WindowRect.left+1, WindowRect.bottom-WindowRect.top+1, TRUE))
            {
                fperror("error:%i:Failed to set the window position to (%i, %i) with Windows System Error Code %lu\n", i, x, y, GetLastError());
                return 1;
            }
        }
        else if (!strcmp("movewindow", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y,
                "error:%i:Option \'movewindow\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x_offset on option \"movewindow\"\n",
                "error:%i:Invalid integer \"%s\" for y_offset on option \"movewindow\"\n") != 0)
                return 1;

            if (!ActuallyMoveWindow(hConsoleWindow, x, y, TRUE))
            {
                fperror("error:%i:Failed to move the console window by (%i, %i) with Windows Error Code %lu\n", i, x, y, GetLastError());
                return 1;
            }
        }
        // Ok so I honestly just dont know how to do this so I'm just gonna remove this for now and then add it back when i figure out how to do it
        /*else if (!strcmp("resizewindow", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y,
                "error:%i:Option \'resizewindow\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for width on option \"resizewindow\"\n",
                "error:%i:Invalid integer \"%s\" for height on option \"resizewindow\"\n") != 0)
                return 1;

            CONSOLE_FONT_INFO cfi;
            if (!GetCurrentConsoleFont(hConsoleOutput, FALSE, &cfi))
            {
                fperror("error:%i:Failed to get the current console font info with Windows Error Code %lu\n", i, GetLastError());
                return 1;
            }
            printf("Thing is (%i, %i) %lu\n", cfi.dwFontSize.X, cfi.dwFontSize.Y, cfi.nFont);
            if (!ResizeWindow(hConsoleWindow, cfi.dwFontSize.X * x, cfi.dwFontSize.Y * y, TRUE))
            {
                fperror("error:%i:Failed to set the console window info with Windows Error Code %lu\n", i, GetLastError());
                return 1;
            }
            //printf("error:%i:so uh, i havent actually implemented this because idk how to do it so uhhhhhh ye i wont close the program just because of this error tho im just gonna give back the arguments for this function for u (%i, %i)\n", i, x, y);
        }*/
        else if (!strcmp("resizewindowpixel", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y,
                "error:%i:Option \'resizewindow\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for width on option \"resizewindow\"\n",
                "error:%i:Invalid integer \"%s\" for height on option \"resizewindow\"\n") != 0)
                return 1;

            if (!ResizeWindow(hConsoleWindow, x, y, TRUE))
            {
                fperror("error:%i:Failed to resize the console window to (%i, %i) with Windows Error Code %lu\n", i, x, y, GetLastError());
                return 1;
            }
        }
        else if (!strcmp("getcurosrpos", argv[i]))
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
            {
                fperror("error:%i:Failed to get the console screen buffer info with Windows Error Code %lu\n", i, GetLastError());
                return 1;
            }
            fprintinfo(i, "The cursor position is (%i, %i)\n", csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y);
        }
        else if (!strcmp("getwindowpos", argv[i]))
        {
            RECT Rect;
            if (!GetWindowRect(hConsoleWindow, &Rect))
            {
                fperror("error:%i:Failed to get the window rect with Windows Error Code %lu\n", i, GetLastError());
                return 1;
            }
            fprintinfo(i, "The window position is (%ld, %ld)\n", Rect.left, Rect.top);
        }
        else if (!strcmp("getwindowsize", argv[i]))
        {
            SMALL_RECT ConsoleWindow;
            if (!GetConsoleWindowRect(hConsoleOutput, &ConsoleWindow))
            {
                fperror("error:%i:Failed to get the console window size (in characters) with Windows Error Code %lu\n", i, GetLastError());
                return 1;
            }
            fprintinfo(i, "The window size is (%i, %i)\n", ConsoleWindow.Right-ConsoleWindow.Left+1, ConsoleWindow.Bottom-ConsoleWindow.Top+1);
        }
        else if (!strcmp("getwindowsizepixel", argv[i]))
        {
            RECT Rect;
            if (!GetWindowRect(hConsoleWindow, &Rect))
            {
                fperror("error:%i:Failed to get the window rect with Windows Error Code %lu\n", i, GetLastError());
                return 1;
            }
            fprintinfo(i, "The window size in pixels is (%ld, %ld)\n", Rect.bottom-Rect.top, Rect.right-Rect.left);
        }
        else
        {
            printf("error:%i:Invalid option \"%s\"\n", i, argv[i]);
            return 1;
        }
    }
    return 0;
}
