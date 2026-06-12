#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <iostream>
#include "Monitor.h"

// Shared flag: console ctrl handler writes, Monitor reads
std::atomic<bool> g_running{ true };

BOOL WINAPI ConsoleCtrlHandler(DWORD type)
{
    switch (type) {
    case CTRL_CLOSE_EVENT:
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        g_running = false;
        Sleep(300); // give main loop time to clean up
        return TRUE;
    }
    return FALSE;
}

static void ConfigureConsole()
{
    SetConsoleTitleA("DataMonitor v1.0 - Real-time Data State Viewer");
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
        SHORT cols = std::max(csbi.dwSize.X, (SHORT)100);
        SHORT rows = std::max(csbi.dwSize.Y, (SHORT)50);
        COORD size{ cols, rows };
        SetConsoleScreenBufferSize(hOut, size);
        SMALL_RECT rect{ 0, 0, (SHORT)(cols - 1), (SHORT)44 };
        SetConsoleWindowInfo(hOut, TRUE, &rect);
    }
}

int main()
{
    ConfigureConsole();

    Monitor monitor(g_running);
    monitor.Run();

    // Restore cursor on clean exit
    CONSOLE_CURSOR_INFO cci{ 10, TRUE };
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &cci);

    return 0;
}
