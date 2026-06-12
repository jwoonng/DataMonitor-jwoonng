#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <vector>
#include "DataStore.h"

struct RenderState {
    const std::vector<DataEntry>* entries = nullptr;
    int scrollOffset    = 0;
    int selectedRow     = 0;
    int okCount         = 0;
    int warnCount       = 0;
    int errCount        = 0;
    int refreshInterval = 2;
    int nextRefreshSec  = 0;
    std::string filterLabel;
    std::string sortLabel;
    bool paused         = false;
};

namespace Colors {
    constexpr WORD NORMAL    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    constexpr WORD BRIGHT    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD OK_COLOR  = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    constexpr WORD WARN_COLOR= FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    constexpr WORD ERR_COLOR = FOREGROUND_RED | FOREGROUND_INTENSITY;
    constexpr WORD TITLE     = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD BORDER    = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD DIM       = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    constexpr WORD SEL_BG    = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
}

class ConsoleUI {
public:
    ConsoleUI();
    void Render(const RenderState& rs);
    int  GetVisibleRows() const;
    void HideCursor();
    void ShowCursor();

private:
    HANDLE hOut_;
    int    width_  = 100;
    int    height_ = 40;

    void GotoXY(int x, int y);
    void SetColor(WORD color);
    void ResetColor();
    void WriteStr(const std::string& s);
    void PrintLine(int y, const std::string& s, WORD color = Colors::NORMAL);
    void DrawBorder(int y, bool isHeader = false);
    void DrawDataRow(int screenY, const DataEntry& e, bool selected);

    std::string Pad(const std::string& s, int w, bool left = false) const;
    std::string FormatTime(time_t t) const;
    WORD        StatusColor(DataStatus s) const;
    std::string StatusStr(DataStatus s) const;

    void UpdateConsoleSize();

    // Fixed column content widths (excluding border chars)
    static constexpr int W_ID   = 3;
    static constexpr int W_KEY  = 26;
    static constexpr int W_VAL  = 15;
    static constexpr int W_TYPE = 7;
    static constexpr int W_STS  = 5;
    static constexpr int W_TIME = 8;
    // Total row width: 7 borders + 6*2 spaces + sum = 7+12+64 = 83
    static constexpr int TABLE_WIDTH = 83;
};
