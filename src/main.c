#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "cstvector.h"

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif // ENABLE_VIRTUAL_TERMINAL_PROCESSING

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

WINBOOL EnableWindowResizing(HWND hWnd, WINBOOL bEnable)
{
    LONG_PTR lWindowStyle = GetWindowLongPtrA(hWnd, GWL_STYLE);
    if (lWindowStyle == 0)
        return FALSE;

    if (bEnable)
        lWindowStyle = lWindowStyle | WS_SIZEBOX | WS_MAXIMIZEBOX;
    else
        lWindowStyle = lWindowStyle & ~WS_SIZEBOX & ~WS_MAXIMIZEBOX;

    if (SetWindowLongPtrA(hWnd, GWL_STYLE, lWindowStyle) == 0)
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

WINBOOL SetWindowMaximized(HWND hWnd, WINBOOL bMaximized)
{
    LONG_PTR lWindowStyle = GetWindowLongPtrA(hWnd, GWL_STYLE);
    if (lWindowStyle == 0)
        return FALSE;

    if (!SetWindowLongPtrA(hWnd, GWL_STYLE, bMaximized ? lWindowStyle | WS_MAXIMIZE : lWindowStyle & WS_MAXIMIZE))
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
int get_two_long_arguments(int *index, int argc, char **argv, unsigned base, long long int *x, long long int *y, int pae, const char *argerror, const char *xerr, const char *yerr)
{
    int i = *index;
    if (argc-i-1 < 2)
    {
        if (pae)
            printf(argerror, i, argc-i-1);

        return 1;
    }
    char *x_str = argv[i+1];
    char *y_str = argv[i+2];
    int xv = strtoll(x_str, &x_str, base), yv = strtoll(y_str, &y_str, base);
    if (*x_str != 0)
    {
        if (pae)
            printf(xerr, i+1, argv[i+1]);

        return 1;
    }
    if (*y_str != 0)
    {
        if (pae)
            printf(yerr, i+2, argv[i+2]);

        return 1;
    }
    *x = xv;
    *y = yv;
    *index += 2;
    return 0;
}

int get_two_int_arguments(int *index, int argc, char **argv, unsigned base, int *x, int *y, int pae, const char *argerror, const char *xerr, const char *yerr)
{
    long long int xl, yl;
    if (get_two_long_arguments(index, argc, argv, base, &xl, &yl, pae, argerror, xerr, yerr) != 0)
        return 1;

    *x = xl;
    *y = yl;
    return 0;
}

typedef struct
{
    int command_index; // For error printing purposes
    enum cmd
    {
        CMD_SETCURSORPOSITION,
        CMD_MOVECURSOR,
        CMD_SETRESIZABLE,
        CMD_ENABLEVT100,
        CMD_SETWINDOWPOSITION,
        CMD_MOVEWINDOW,
        CMD_RESIZEWINDOWPIXEL,
        CMD_GETCURSORPOS,
        CMD_GETWINDOWPOS,
        CMD_GETWINDOWSIZE,
        CMD_GETWINDOWSIZEPIXEL,
        CMD_ISWINDOWMAXIMIZED
    } command_id;
    union
    {
        struct
        {
            int x, y;
        } two_ints;
        int integer; // Can also be used as a boolean
        const char *string;
        int boolean;
    } argument;
} command;

int execute_commands(cstvector_t *commandvec, int pe, int pae)
{
    HANDLE hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    HWND hConsoleWindow = GetConsoleWindow();
    if (hConsoleOutput == NULL)
    {
        if (pe)
            fperror("Failed to get the console screen buffer handle thing with Windows System Error Code %lu\n", GetLastError());

        return 2;
    }
    if (hConsoleWindow == NULL)
    {
        if (pe)
            fperror("Failed to get the console window handle with Windows System Error Code %lu\n", GetLastError());

        return 2;
    }
    command *curcmdptr;
    for (size_t i = 0; i < commandvec->size; ++i)
    {
        if ((curcmdptr = cstvector_getat(commandvec, i)) == NULL)
        {
            if (pe)
                fperror("error:command_%I64u:Failed to get index %I64u from the vector with errno %i (%s)\n", i, i, errno, strerror(errno));

            return 3;
        }

        switch (curcmdptr->command_id)
        {
        case CMD_SETCURSORPOSITION:
            if (!SetConsoleCursorPosition(hConsoleOutput, CreateCoord(curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y)))
            {
                if (pe)
                    fperror("error:%i:Failed to set the console cursor position to (%i, %i) with Windows System Error Code %lu\n", curcmdptr->command_index, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, GetLastError());

                return 2;
            }
            break;
        case CMD_MOVECURSOR:
            {
                COORD console_cursor_position;
                if (!GetConsoleCursorPosition(hConsoleOutput, &console_cursor_position))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the console's cursor position on option \"movecursor\" with Windows System Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                printf("%i %i\n", curcmdptr->argument.two_ints.y, console_cursor_position.Y + curcmdptr->argument.two_ints.y);
                if (!SetConsoleCursorPosition(hConsoleOutput, CreateCoord(console_cursor_position.X + curcmdptr->argument.two_ints.x, console_cursor_position.Y + curcmdptr->argument.two_ints.y)))
                {
                    if (pe)
                        fperror("error:%i:Failed to move the console cursor position by (%i, %i) with Windows System Error Code %lu\n", curcmdptr->command_index, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, GetLastError());

                    return 2;
                }
            }
            break;
        case CMD_SETRESIZABLE:
            if (!EnableWindowResizing(hConsoleWindow, curcmdptr->argument.boolean))
            {
                if (pe)
                    fperror("error:%i:Failed to enable console resizing with Windows System Error Code %lu\n", curcmdptr->command_index, GetLastError());

                return 2;
            }
            break;
        case CMD_SETWINDOWPOSITION:
            {
                RECT WindowRect;
                if (!GetWindowRect(hConsoleWindow, &WindowRect))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the console window rect with Windows System Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                if (!MoveWindow(hConsoleWindow, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, WindowRect.right-WindowRect.left+1, WindowRect.bottom-WindowRect.top+1, TRUE))
                {
                    if (pe)
                        fperror("error:%i:Failed to set the window position to (%i, %i) with Windows System Error Code %lu\n", curcmdptr->command_index, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, GetLastError());

                    return 2;
                }
            }
            break;
        case CMD_MOVEWINDOW:
            if (!ActuallyMoveWindow(hConsoleWindow, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, TRUE))
            {
                if (pe)
                    fperror("error:%i:Failed to move the console window by (%i, %i) with Windows Error Code %lu\n", curcmdptr->command_index, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, GetLastError());

                return 2;
            }
            break;
        case CMD_RESIZEWINDOWPIXEL:
            if (!ResizeWindow(hConsoleWindow, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, TRUE))
            {
                if (pe)
                    fperror("error:%i:Failed to resize the console window to (%i, %i) with Windows Error Code %lu\n", curcmdptr->command_index, curcmdptr->argument.two_ints.x, curcmdptr->argument.two_ints.y, GetLastError());

                return 2;
            }
            break;
        case CMD_GETCURSORPOS:
            {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                if (!GetConsoleScreenBufferInfo(hConsoleOutput, &csbi))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the console screen buffer info with Windows Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                fprintinfo(curcmdptr->command_index, "The cursor position is (%i, %i)\n", csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y);
                break;
            }
        case CMD_GETWINDOWPOS:
            {
            RECT Rect;
                if (!GetWindowRect(hConsoleWindow, &Rect))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the window rect with Windows Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                fprintinfo(curcmdptr->command_index, "The window position is (%ld, %ld)\n", Rect.left, Rect.top);
            }
            break;
        case CMD_GETWINDOWSIZE:
            {
                SMALL_RECT ConsoleWindow;
                if (!GetConsoleWindowRect(hConsoleOutput, &ConsoleWindow))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the console window size (in characters) with Windows Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                fprintinfo(curcmdptr->command_index, "The window size is (%i, %i)\n", ConsoleWindow.Right-ConsoleWindow.Left+1, ConsoleWindow.Bottom-ConsoleWindow.Top+1);
                break;
            }
        case CMD_GETWINDOWSIZEPIXEL:
            {
                RECT Rect;
                if (!GetWindowRect(hConsoleWindow, &Rect))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the window rect with Windows Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                fprintinfo(curcmdptr->command_index, "The window size in pixels is (%ld, %ld)\n", Rect.right-Rect.left, Rect.bottom-Rect.top);
            }
            break;
        case CMD_ISWINDOWMAXIMIZED:
            {
                LONG_PTR lConsoleWindowStyle;
                if (!(lConsoleWindowStyle = GetWindowLongPtrA(hConsoleWindow, GWL_STYLE)))
                {
                    if (pe)
                        fperror("error:%i:Failed to get the window long ptr of the console with Windows Error Code %lu\n", curcmdptr->command_index, GetLastError());

                    return 2;
                }
                fprintinfo(curcmdptr->command_index, "The window is %s\n", lConsoleWindowStyle & WS_MAXIMIZE ? "maximized" : "not maximized");
            }
            break;
        default:
            return -2;
        }
    }
    return 0;
}

int strtobool(char *str, char *ye, char *no)
{
    if (alphabet_caseinsensitive_strcmp(str, ye))
        return 1;

    if (alphabet_caseinsensitive_strcmp(str, no))
        return 0;

    return -1;
}

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        printf("Changer options(what does that even mean lol):\n"
               "    -nb (number base) <base>\n"
               "        Change the base number system (or whatever) of the program, any arguments with type integer in them will will receive a number with the selected base (the default base is base 10) (this function always uses base 10)\n\n"
               "    -pe (print error) <true|false>\n"
               "        Disable printing (errno or WINAPI) errors (if an error happens, the program will return a non-zero value, this flag will be default to true\n\n"
               "    -pae (print argument error) <true|false>\n"
               "        Disable printing argument errors (argument errors are errors from invalid arguments), this flag will be default to true\n\n"
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
               "    getcursorpos\n"
               "        Prints the cursor position\n\n"
               "    getwindowpos\n"
               "        Prints the console window position\n\n"
               "    getwindowsize\n"
               "        Prints the window size in (column, rows)\n\n"
               "    getwindowsizepixel\n"
               "        Prints the window size in pixel\n\n"
               "    iswindowmaximized\n"
               "        Prints if the window is maximized or not\n\n"
               "Additional Information:\n"
               "    The error format will be like this:\n"
               "        error:[argument position]: [error] with [\"errno\" or \"Window System Error Code\"] [error code] [what the error means if its not a windows system error code]\n\n"
               "    The print format will be like this:\n"
               "        info:[argument position]:[print type] is [value]\n\n"
               "    Error return values:\n"
               "       1 means that you have passed an invalid argument\n"
               "       2 means that a WINAPI error occured\n"
               "       3 means that a non-WINAPI error occured (some kind of errno error)\n\n"
               "    The program will only run the option(s) if there is no invalid argument error(s) (so it basically changes all of the options to tokens and then execute them in another function if all of the program arguments are correct).\n\n");
        return 0;
    }
    cstvector_t cmdvec;
    if (cstvector_init(&cmdvec, sizeof(command)))
    {
        fperror("Failed to initialize vector with errno %i (%s)\n", errno, strerror(errno));
        return 3;
    }
    unsigned base = 10;
    int print_errors = 1;
    int print_argument_errors = 1;
    for (int i = 1; i < argc; ++i)
    {
        command curcmd;
        curcmd.command_index = i;
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
            else if (argv[i][1] == 'p' && argv[i][2] == 'e')
            {
                if (argc-i-1 < 1)
                {
                    printf("Option \'-pe\' needs 1 additional argument, but got %i\n", argc-i-1);
                    return 1;
                }
                // The 'i' increment is in hereeee
                if ((print_argument_errors = strtobool(argv[++i], "true", "false")) == -1)
                {
                    if (print_argument_errors)
                        printf("error:%i:Invalid boolean value \"%s\" on option \'pe\'\n", i, argv[i]);

                    return 1;
                }
            }
            else if (argv[i][1] == 'p' && argv[i][2] == 'a' && argv[i][3] == 'e')
            {
                if (argc-i-1 < 1)
                {
                    printf("Option \'-pae\' needs 1 additional argument, but got %i\n", argc-i-1);
                    return 1;
                }
                if ((print_argument_errors = strtobool(argv[++i], "true", "false")) == -1)
                {
                    if (print_argument_errors)
                        printf("error:%i:Invalid boolean value \"%s\" on option \'pae\'\n", i, argv[i]);

                    return 1;
                }
            }
            else
            {
                printf("error:%i:Invalid changer option (or whatever) \"%s\"\n", i, argv[i]);
                return 1;
            }
            continue;
        }
        else if (!strcmp("setcursorposition", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y, print_argument_errors,
                "error:%i:Option \'setcursorpos\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x on option \"setcursorpos\"\n",
                "error:%i:Invalid integer \"%s\" for y on option \"setcursorpos\"\n") != 0)
                return 1;

            curcmd.argument.two_ints.x = x;
            curcmd.argument.two_ints.y = y;
            curcmd.command_id = CMD_SETCURSORPOSITION;
        }
        else if (!strcmp("movecursor", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y, print_argument_errors,
                "error:%i:Option \'movecursor\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x_offset on option \"movecursor\"\n",
                "error:%i:Invalid integer \"%s\" for y_offset on option \"movecursor\"\n") != 0)
                return 1;

            curcmd.argument.two_ints.x = x;
            curcmd.argument.two_ints.y = y;
            curcmd.command_id = CMD_MOVECURSOR;
        }
        else if (!strcmp("setresizable", argv[i]))
        {
            if (argc-i-1 < 1)
            {
                printf("error:%i:Option \'setresizable\' needs 1 additional argument, but got %i\n", i, argc-i-1);
                return 1;
            }
            if ((curcmd.argument.boolean = strtobool(argv[i+1], "true", "false")) == -1)
            {
                if (print_argument_errors)
                    printf("error:%i:Invalid boolean value \"%s\" on option \'setresizable\'\n", i, argv[i+1]);

                return 1;
            }
            curcmd.command_id = CMD_SETRESIZABLE;
            ++i;
        }
        else if (!strcmp("setwindowposition", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y, print_argument_errors,
                "error:%i:Option \'setwindowposition\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x on option \"setwindowposition\"\n",
                "error:%i:Invalid integer \"%s\" for y on option \"setwindowposition\"\n") != 0)
                return 1;

            curcmd.argument.two_ints.x = x;
            curcmd.argument.two_ints.y = y;
            curcmd.command_id = CMD_SETWINDOWPOSITION;
        }
        else if (!strcmp("movewindow", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y, print_argument_errors,
                "error:%i:Option \'movewindow\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for x_offset on option \"movewindow\"\n",
                "error:%i:Invalid integer \"%s\" for y_offset on option \"movewindow\"\n") != 0)
                return 1;

            curcmd.argument.two_ints.x = x;
            curcmd.argument.two_ints.y = y;
            curcmd.command_id = CMD_MOVEWINDOW;
        }
        // Ok so I honestly just dont know how to do this so I'm just gonna remove this for now and then add it back when i figure out how to do it
        /*else if (!strcmp("resizewindow", argv[i]))
        {
            int x, y;
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y, print_argument_errors,
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
            if (get_two_int_arguments(&i, argc, argv, base, &x, &y, print_argument_errors,
                "error:%i:Option \'resizewindow\' needs 2 additional argument, but got %i\n",
                "error:%i:Invalid integer \"%s\" for width on option \"resizewindow\"\n",
                "error:%i:Invalid integer \"%s\" for height on option \"resizewindow\"\n") != 0)
                return 1;

            curcmd.argument.two_ints.x = x;
            curcmd.argument.two_ints.y = y;
            curcmd.command_id = CMD_RESIZEWINDOWPIXEL;
        }
        /*else if (!strcmp("setwindowmaximized", argv[i]))
        {
            if (argc-i-1 < 1)
            {
                printf("error:%i:Option \'setwindowmaximized\' needs 1 additional argument, but got %i\n", i, argc-i-1);
                return 1;
            }
            const char *arg = argv[i+1];
            if (alphabet_caseinsensitive_strcmp(arg, "true"))
            {
                curcmd.argument.boolean = 1;
            }
            else if (alphabet_caseinsensitive_strcmp(arg, "false"))
            {
                curcmd.argument.boolean = 0;
            }
            else
            {
                if (print_argument_errors)
                    printf("error:%i:Invalid boolean value \"%s\" on option \'setwindowmaximized\'", i, arg);

                return 1;
            }
            curcmd.command_id = CMD_SETWINDOWMAXIMIZED;
            ++i;
        }*/
        else if (!strcmp("getcursorpos", argv[i]))
        {
            curcmd.command_id = CMD_GETCURSORPOS;
        }
        else if (!strcmp("getwindowpos", argv[i]))
        {
            curcmd.command_id = CMD_GETWINDOWPOS;
        }
        else if (!strcmp("getwindowsize", argv[i]))
        {
            curcmd.command_id = CMD_GETWINDOWSIZE;
        }
        else if (!strcmp("getwindowsizepixel", argv[i]))
        {
            curcmd.command_id = CMD_GETWINDOWSIZEPIXEL;
        }
        else if (!strcmp("iswindowmaximized", argv[i]))
        {
            curcmd.command_id = CMD_ISWINDOWMAXIMIZED;
        }
        else
        {
            printf("error:%i:Invalid option \"%s\"\n", i, argv[i]);
            return 1;
        }
        if (cstvector_push_back(&cmdvec, &curcmd) != 0)
        {
            fperror("Failed to push back the current command to the vector with errno %i (%s)\n", errno, strerror(errno));
            return 3;
        }
    }
    // Execute commands return value
    int ecrv;
    ecrv = execute_commands(&cmdvec, print_errors, print_argument_errors);
    cstvector_destroy(&cmdvec);
    return ecrv;
}
