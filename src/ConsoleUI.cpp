#include "ConsoleUI.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

// separator line: "  ----  --------  ..."
static const std::string SEP =
    "  " + std::string(4,  '-') + "  "
         + std::string(30, '-') + "  "
         + std::string(15, '-') + "  "
         + std::string(8,  '-') + "  "
         + std::string(5,  '-') + "  "
         + std::string(8,  '-');

ConsoleUI::ConsoleUI()
    : hOut_(GetStdHandle(STD_OUTPUT_HANDLE))
{
    UpdateSize();
    HideCursor();
}

void ConsoleUI::HideCursor()
{
    CONSOLE_CURSOR_INFO c{ 1, FALSE };
    SetConsoleCursorInfo(hOut_, &c);
}

void ConsoleUI::ShowCursor()
{
    CONSOLE_CURSOR_INFO c{ 10, TRUE };
    SetConsoleCursorInfo(hOut_, &c);
}

int ConsoleUI::GetVisibleRows() const
{
    // fixed overhead: 1(blank)+1(title)+1(blank)+1(info)+1(sep)+1(hdr)+1(sep) = 7 top
    //                 1(sep)+1(help)+1(blank) = 3 bottom
    return std::max(1, height_ - 10);
}

// ---------------------------------------------------------------------------
void ConsoleUI::Render(const RenderState& rs)
{
    UpdateSize();
    GotoXY(0, 0);

    const auto& entries = *rs.entries;
    int visRows = GetVisibleRows();

    // Line 0 – blank buffer line
    PrintPadded(0, "");

    // Line 1 – title bar
    {
        time_t now = std::time(nullptr);
        tm lt{};
        localtime_s(&lt, &now);
        char tbuf[32];
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &lt);

        std::ostringstream ss;
        ss << "  DataMonitor v1.0  |  " << tbuf << "  |  ";
        if (rs.paused)
            ss << "[ PAUSED ]";
        else
            ss << "Refresh: " << rs.refreshInterval << "s  (next: " << rs.nextRefreshSec << "s)";

        PrintPadded(1, ss.str(), Color::CYAN);
    }

    // Line 2 – blank
    PrintPadded(2, "");

    // Line 3 – filter / sort / page / status summary
    {
        int total    = (int)entries.size();
        int pageNum  = (visRows > 0 && total > 0) ? rs.scrollOffset / visRows + 1 : 1;
        int pageTotal= (visRows > 0 && total > 0) ? (total + visRows - 1) / visRows : 1;

        std::ostringstream ss;
        ss << "  Filter: " << rs.filterLabel
           << "  Sort: "   << rs.sortLabel
           << "  Page: "   << pageNum << "/" << pageTotal
           << "  Entries: " << total;

        // Build right-side status counts string
        std::string left = ss.str();
        std::string okStr   = "OK:"   + std::to_string(rs.okCount);
        std::string warnStr = "WARN:" + std::to_string(rs.warnCount);
        std::string errStr  = "ERR:"  + std::to_string(rs.errCount);
        std::string right   = okStr + "  " + warnStr + "  " + errStr + "  ";

        int gap = width_ - (int)left.size() - (int)right.size();
        if (gap < 1) gap = 1;

        GotoXY(0, 3);
        SetColor(Color::DIM);   Print(left + std::string(gap, ' '));
        SetColor(Color::OK);    Print(okStr);
        SetColor(Color::DIM);   Print("  ");
        SetColor(Color::WARN);  Print(warnStr);
        SetColor(Color::DIM);   Print("  ");
        SetColor(Color::ERR);   Print(errStr);
        SetColor(Color::DIM);   Print("  ");
        SetColor(Color::NORMAL);
    }

    // Line 4 – separator
    PrintPadded(4, SEP, Color::DIM);

    // Line 5 – column header
    {
        std::string hdr = "  "
            + Fit("ID",      W_ID,   true) + "  "
            + Fit("KEY",     W_KEY)        + "  "
            + Fit("VALUE",   W_VAL)        + "  "
            + Fit("TYPE",    W_TYPE)       + "  "
            + Fit("STS",     W_STS)        + "  "
            + Fit("UPDATED", W_TIME);
        PrintPadded(5, hdr, Color::BRIGHT);
    }

    // Line 6 – separator
    PrintPadded(6, SEP, Color::DIM);

    // Lines 7..7+visRows-1 – data
    int dataY = 7;
    int end   = std::min(rs.scrollOffset + visRows, (int)entries.size());

    int screenY = dataY;
    for (int i = rs.scrollOffset; i < end; ++i, ++screenY)
        DrawDataRow(screenY, entries[i], i == rs.selectedRow);

    // Empty filler rows
    for (; screenY < dataY + visRows; ++screenY)
        PrintPadded(screenY, "");

    // Separator after data
    PrintPadded(dataY + visRows, SEP, Color::DIM);

    // Help line
    PrintPadded(dataY + visRows + 1,
        "  [Q]Quit  [R]Refresh  [P]Pause  [F]Filter  [S]Sort  [+/-]Interval  [Up/Dn]Scroll  [Home/End]",
        Color::DIM);

    SetColor(Color::NORMAL);
}

// ---------------------------------------------------------------------------
void ConsoleUI::DrawDataRow(int y, const DataEntry& e, bool selected)
{
    GotoXY(0, y);

    bool sel = selected;
    auto c = [&](WORD col) { SetColor(sel ? Color::SEL : col); };

    // cursor indicator
    c(Color::DIM);
    Print(sel ? " > " : "   ");

    c(Color::DIM);    Print(Fit(std::to_string(e.id), W_ID, true) + "  ");
    c(sel ? Color::SEL : Color::NORMAL); Print(Fit(e.key,  W_KEY) + "  ");
    c(sel ? Color::SEL : Color::NORMAL); Print(Fit(e.value, W_VAL) + "  ");
    c(Color::DIM);    Print(Fit(e.typeStr, W_TYPE) + "  ");

    // Status with color
    SetColor(sel ? Color::SEL : StatusColor(e.status));
    Print(Fit(StatusStr(e.status), W_STS) + "  ");

    c(Color::DIM);    Print(Fit(FormatTime(e.lastUpdated), W_TIME));

    // Pad to end of line
    int used = 3 + W_ID+2 + W_KEY+2 + W_VAL+2 + W_TYPE+2 + W_STS+2 + W_TIME;
    if (width_ - used > 0) { SetColor(sel ? Color::SEL : Color::NORMAL); Print(std::string(width_ - used, ' ')); }
    SetColor(Color::NORMAL);
}

// ---------------------------------------------------------------------------
void ConsoleUI::UpdateSize()
{
    CONSOLE_SCREEN_BUFFER_INFO i;
    if (GetConsoleScreenBufferInfo(hOut_, &i)) {
        width_  = i.srWindow.Right  - i.srWindow.Left + 1;
        height_ = i.srWindow.Bottom - i.srWindow.Top  + 1;
    }
}

void ConsoleUI::GotoXY(int x, int y)
{
    COORD c{ (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(hOut_, c);
}

void ConsoleUI::SetColor(WORD c) { SetConsoleTextAttribute(hOut_, c); }

void ConsoleUI::Print(const std::string& s)
{
    DWORD w;
    WriteConsoleA(hOut_, s.c_str(), (DWORD)s.size(), &w, nullptr);
}

void ConsoleUI::PrintColored(const std::string& s, WORD c)
{
    SetColor(c);
    Print(s);
    SetColor(Color::NORMAL);
}

void ConsoleUI::PrintLine(int y, const std::string& s, WORD c)
{
    GotoXY(0, y);
    SetColor(c);
    Print(s);
    SetColor(Color::NORMAL);
}

void ConsoleUI::PrintPadded(int y, const std::string& s, WORD c)
{
    GotoXY(0, y);
    SetColor(c);
    int sz = (int)s.size();
    if (sz >= width_) Print(s.substr(0, width_));
    else { Print(s); Print(std::string(width_ - sz, ' ')); }
    SetColor(Color::NORMAL);
}

std::string ConsoleUI::Fit(const std::string& s, int w, bool right) const
{
    int len = (int)s.size();
    if (len >= w) return s.substr(0, w);
    if (right)    return std::string(w - len, ' ') + s;
    return s + std::string(w - len, ' ');
}

std::string ConsoleUI::FormatTime(time_t t) const
{
    tm lt{};
    localtime_s(&lt, &t);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &lt);
    return buf;
}

WORD ConsoleUI::StatusColor(DataStatus s) const
{
    switch (s) {
    case DataStatus::OK:   return Color::OK;
    case DataStatus::WARN: return Color::WARN;
    case DataStatus::ERR:  return Color::ERR;
    }
    return Color::NORMAL;
}

std::string ConsoleUI::StatusStr(DataStatus s) const
{
    switch (s) {
    case DataStatus::OK:   return "OK";
    case DataStatus::WARN: return "WARN";
    case DataStatus::ERR:  return "ERR";
    }
    return "?";
}
