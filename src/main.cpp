#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <iostream>
#include "Monitor.h"

static void ConfigureConsole()
{
    SetConsoleTitleA("DataMonitor v1.0 - Real-time Data State Viewer");

    // Resize console window to at least 90x40
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        SHORT cols = std::max(csbi.dwSize.X, (SHORT)100);
        SHORT rows = std::max(csbi.dwSize.Y, (SHORT)50);
        COORD size{ cols, rows };
        SetConsoleScreenBufferSize(hOut, size);

        SMALL_RECT rect{ 0, 0, static_cast<SHORT>(cols - 1), (SHORT)44 };
        SetConsoleWindowInfo(hOut, TRUE, &rect);
    }
}

int main()
{
    ConfigureConsole();

    Monitor monitor;
    monitor.Run();

    // Restore cursor and clear on exit
    CONSOLE_CURSOR_INFO cci{ 10, TRUE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);
    std::cout << "\n\nDataMonitor terminated.\n";

    return 0;
}
