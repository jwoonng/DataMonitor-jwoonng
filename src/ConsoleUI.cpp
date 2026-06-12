#include "ConsoleUI.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <ctime>

ConsoleUI::ConsoleUI()
    : hOut_(GetStdHandle(STD_OUTPUT_HANDLE))
{
    UpdateConsoleSize();
    HideCursor();
}

// ---------------------------------------------------------------------------
// Public
// ---------------------------------------------------------------------------
void ConsoleUI::HideCursor()
{
    CONSOLE_CURSOR_INFO cci{ 1, FALSE };
    SetConsoleCursorInfo(hOut_, &cci);
}

void ConsoleUI::ShowCursor()
{
    CONSOLE_CURSOR_INFO cci{ 10, TRUE };
    SetConsoleCursorInfo(hOut_, &cci);
}

int ConsoleUI::GetVisibleRows() const
{
    // header=5, footer=3, separator row per side, summary=1
    return std::max(1, height_ - 10);
}

void ConsoleUI::Render(const RenderState& rs)
{
    UpdateConsoleSize();
    GotoXY(0, 0);

    const auto& entries = *rs.entries;
    int visRows = GetVisibleRows();

    // ---- Title bar -------------------------------------------------------
    {
        std::ostringstream title;
        title << "  DataMonitor v1.0  |  ";

        // Current time
        time_t now = std::time(nullptr);
        tm   lt{};
        localtime_s(&lt, &now);
        char timebuf[32];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &lt);
        title << timebuf << "  |  ";

        if (rs.paused)
            title << "[ PAUSED ]";
        else
            title << "Refresh: " << rs.refreshInterval << "s  (next: " << rs.nextRefreshSec << "s)";

        PrintLine(0, "", Colors::TITLE);
        std::string tl = "+" + std::string(TABLE_WIDTH - 2, '-') + "+";
        PrintLine(1, tl, Colors::BORDER);

        std::string tstr = title.str();
        int padding = TABLE_WIDTH - 2 - (int)tstr.size();
        if (padding < 0) { tstr = tstr.substr(0, TABLE_WIDTH - 2); padding = 0; }
        std::string titleLine = "|" + tstr + std::string(padding, ' ') + "|";
        PrintLine(2, titleLine, Colors::TITLE);
        PrintLine(3, tl, Colors::BORDER);
    }

    // ---- Filter / sort / page info ----------------------------------------
    {
        std::ostringstream info;
        info << "  Filter: [" << rs.filterLabel << "]"
             << "  Sort: [" << rs.sortLabel << "]"
             << "  Page: " << (entries.empty() ? 0 : rs.scrollOffset / visRows + 1)
             << " / " << (entries.empty() ? 1 : ((int)entries.size() + visRows - 1) / visRows)
             << "  Entries: " << (int)entries.size();
        std::string s = info.str();
        int pad = TABLE_WIDTH - 2 - (int)s.size();
        if (pad < 0) { s = s.substr(0, TABLE_WIDTH - 2); pad = 0; }
        PrintLine(4, "|" + s + std::string(pad, ' ') + "|", Colors::DIM);
    }

    // ---- Column header row ------------------------------------------------
    {
        std::string sep = "+" + std::string(W_ID + 2, '-')   + "+"
                              + std::string(W_KEY + 2, '-')  + "+"
                              + std::string(W_VAL + 2, '-')  + "+"
                              + std::string(W_TYPE + 2, '-') + "+"
                              + std::string(W_STS + 2, '-')  + "+"
                              + std::string(W_TIME + 2, '-') + "+";
        PrintLine(5, sep, Colors::BORDER);

        std::string hdr = "| " + Pad("ID",   W_ID)
                        + " | " + Pad("KEY",     W_KEY)
                        + " | " + Pad("VALUE",   W_VAL)
                        + " | " + Pad("TYPE",    W_TYPE)
                        + " | " + Pad("STS",     W_STS)
                        + " | " + Pad("UPDATED", W_TIME)
                        + " |";
        PrintLine(6, hdr, Colors::BRIGHT);
        PrintLine(7, sep, Colors::BORDER);
    }

    // ---- Data rows --------------------------------------------------------
    int dataStartY = 8;
    int end = std::min(rs.scrollOffset + visRows, (int)entries.size());
    int row = rs.scrollOffset;
    int screenY = dataStartY;

    for (; row < end; ++row, ++screenY)
        DrawDataRow(screenY, entries[row], row == rs.selectedRow);

    // Fill empty rows below data
    std::string emptyRow = "|" + std::string(TABLE_WIDTH - 2, ' ') + "|";
    for (; screenY < dataStartY + visRows; ++screenY)
        PrintLine(screenY, emptyRow, Colors::BORDER);

    // ---- Summary row -------------------------------------------------------
    int summaryY = dataStartY + visRows;
    {
        std::string botSep = "+" + std::string(TABLE_WIDTH - 2, '-') + "+";
        PrintLine(summaryY, botSep, Colors::BORDER);

        std::ostringstream ss;
        ss << "  Status Summary:  ";

        // Print OK count (green), WARN count (yellow), ERR count (red)
        GotoXY(0, summaryY + 1);
        SetColor(Colors::BORDER); WriteStr("|");
        SetColor(Colors::DIM);    WriteStr("  Status: ");
        SetColor(Colors::OK_COLOR);   WriteStr(" OK:" + std::to_string(rs.okCount) + " ");
        SetColor(Colors::WARN_COLOR); WriteStr(" WARN:" + std::to_string(rs.warnCount) + " ");
        SetColor(Colors::ERR_COLOR);  WriteStr(" ERR:" + std::to_string(rs.errCount) + " ");

        // Pad rest of line
        int used = 2 + 8 + (5 + (int)std::to_string(rs.okCount).size())
                         + (7 + (int)std::to_string(rs.warnCount).size())
                         + (6 + (int)std::to_string(rs.errCount).size());
        int rem = TABLE_WIDTH - 2 - used;
        if (rem > 0) { SetColor(Colors::DIM); WriteStr(std::string(rem, ' ')); }
        SetColor(Colors::BORDER); WriteStr("|");
        ResetColor();
    }

    // ---- Help footer -------------------------------------------------------
    {
        int footY = summaryY + 2;
        std::string sep = "+" + std::string(TABLE_WIDTH - 2, '-') + "+";
        PrintLine(footY, sep, Colors::BORDER);

        std::string help1 = "  [Q]Quit  [R]Refresh  [P]Pause  [F]Filter  [S]Sort  [+/-]Interval  [Up/Dn]Scroll";
        int p1 = TABLE_WIDTH - 2 - (int)help1.size();
        if (p1 < 0) { help1 = help1.substr(0, TABLE_WIDTH - 2); p1 = 0; }
        PrintLine(footY + 1, "|" + help1 + std::string(p1, ' ') + "|", Colors::DIM);
        PrintLine(footY + 2, sep, Colors::BORDER);
    }

    ResetColor();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------
void ConsoleUI::UpdateConsoleSize()
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(hOut_, &csbi)) {
        width_  = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
        height_ = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
    }
}

void ConsoleUI::GotoXY(int x, int y)
{
    COORD c{ static_cast<SHORT>(x), static_cast<SHORT>(y) };
    SetConsoleCursorPosition(hOut_, c);
}

void ConsoleUI::SetColor(WORD color)
{
    SetConsoleTextAttribute(hOut_, color);
}

void ConsoleUI::ResetColor()
{
    SetConsoleTextAttribute(hOut_, Colors::NORMAL);
}

void ConsoleUI::WriteStr(const std::string& s)
{
    DWORD written;
    WriteConsoleA(hOut_, s.c_str(), static_cast<DWORD>(s.size()), &written, nullptr);
}

void ConsoleUI::PrintLine(int y, const std::string& s, WORD color)
{
    GotoXY(0, y);
    SetColor(color);
    // Pad or truncate to TABLE_WIDTH
    int sz = (int)s.size();
    if (sz >= TABLE_WIDTH) {
        WriteStr(s.substr(0, TABLE_WIDTH));
    } else {
        WriteStr(s);
        WriteStr(std::string(TABLE_WIDTH - sz, ' '));
    }
    ResetColor();
}

void ConsoleUI::DrawDataRow(int screenY, const DataEntry& e, bool selected)
{
    GotoXY(0, screenY);

    WORD rowColor = selected ? Colors::SEL_BG : Colors::NORMAL;
    WORD stsColor = selected ? Colors::SEL_BG : StatusColor(e.status);

    auto w = [&](WORD c) { SetColor(selected ? Colors::SEL_BG : c); };

    w(Colors::BORDER);  WriteStr("|");
    w(Colors::DIM);     WriteStr(" " + Pad(std::to_string(e.id), W_ID, true) + " ");
    w(Colors::BORDER);  WriteStr("|");
    w(rowColor);        WriteStr(" " + Pad(e.key, W_KEY) + " ");
    w(Colors::BORDER);  WriteStr("|");
    w(rowColor);        WriteStr(" " + Pad(e.value, W_VAL) + " ");
    w(Colors::BORDER);  WriteStr("|");
    w(Colors::DIM);     WriteStr(" " + Pad(e.typeStr, W_TYPE) + " ");
    w(Colors::BORDER);  WriteStr("|");
    w(stsColor);        WriteStr(" " + Pad(StatusStr(e.status), W_STS) + " ");
    w(Colors::BORDER);  WriteStr("|");
    w(Colors::DIM);     WriteStr(" " + Pad(FormatTime(e.lastUpdated), W_TIME) + " ");
    w(Colors::BORDER);  WriteStr("|");

    ResetColor();
}

std::string ConsoleUI::Pad(const std::string& s, int w, bool leftAlign) const
{
    int len = (int)s.size();
    if (len >= w) return s.substr(0, w);
    if (leftAlign) return std::string(w - len, ' ') + s;
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
    case DataStatus::OK:   return Colors::OK_COLOR;
    case DataStatus::WARN: return Colors::WARN_COLOR;
    case DataStatus::ERR:  return Colors::ERR_COLOR;
    }
    return Colors::NORMAL;
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
