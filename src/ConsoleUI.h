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

namespace Color {
    constexpr WORD NORMAL = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    constexpr WORD BRIGHT = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD OK     = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    constexpr WORD WARN   = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    constexpr WORD ERR    = FOREGROUND_RED | FOREGROUND_INTENSITY;
    constexpr WORD DIM    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    constexpr WORD SEL    = BACKGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD CYAN   = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
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

    void   GotoXY(int x, int y);
    void   SetColor(WORD c);
    void   Print(const std::string& s);
    void   PrintColored(const std::string& s, WORD c);
    void   PrintLine(int y, const std::string& s, WORD c = Color::NORMAL);
    void   PrintPadded(int y, const std::string& s, WORD c = Color::NORMAL);
    void   DrawDataRow(int y, const DataEntry& e, bool selected);

    std::string Fit(const std::string& s, int w, bool rightAlign = false) const;
    std::string FormatTime(time_t t) const;
    WORD        StatusColor(DataStatus s) const;
    std::string StatusStr(DataStatus s) const;
    void        UpdateSize();

    // Column content widths
    static constexpr int W_ID   = 4;
    static constexpr int W_KEY  = 30;
    static constexpr int W_VAL  = 15;
    static constexpr int W_TYPE = 8;
    static constexpr int W_STS  = 5;
    static constexpr int W_TIME = 8;
};
