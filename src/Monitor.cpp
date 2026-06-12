#include "Monitor.h"
#include <conio.h>
#include <windows.h>
#include <algorithm>

Monitor::Monitor(std::atomic<bool>& running)
    : running_(running)
    , lastRefresh_(std::chrono::steady_clock::now())
{
    ApplyFilterAndSort();
}

void Monitor::Run()
{
    Refresh();

    while (running_) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 0 || ch == 0xE0) {
                int ch2 = _getch();
                HandleInput(0xE000 | ch2);
            } else {
                HandleInput(ch);
            }
        }

        if (!paused_) {
            auto now = std::chrono::steady_clock::now();
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - lastRefresh_).count();
            if (sec >= refreshInterval_) Refresh();
        }

        Sleep(50);
    }

    // Restore cursor before exit
    ui_.ShowCursor();
}

void Monitor::HandleInput(int ch)
{
    constexpr int KEY_UP   = 0xE048;
    constexpr int KEY_DOWN = 0xE050;
    constexpr int KEY_PGUP = 0xE049;
    constexpr int KEY_PGDN = 0xE051;
    constexpr int KEY_HOME = 0xE047;
    constexpr int KEY_END  = 0xE04F;

    switch (ch) {
    case 'q': case 'Q': running_ = false; break;
    case 'r': case 'R': Refresh(); break;
    case 'p': case 'P': paused_ = !paused_; break;

    case 'f': case 'F':
        filter_ = static_cast<FilterMode>((static_cast<int>(filter_) + 1) % 4);
        scrollOffset_ = 0; selectedRow_ = 0;
        ApplyFilterAndSort();
        break;

    case 's': case 'S':
        sort_ = static_cast<SortMode>((static_cast<int>(sort_) + 1) % 4);
        ApplyFilterAndSort();
        break;

    case '+': case '=': if (refreshInterval_ < 60) ++refreshInterval_; break;
    case '-': case '_': if (refreshInterval_ >  1) --refreshInterval_; break;

    case KEY_UP:   ScrollUp();   break;
    case KEY_DOWN: ScrollDown(); break;
    case KEY_PGUP: PageUp();     break;
    case KEY_PGDN: PageDown();   break;

    case KEY_HOME:
        scrollOffset_ = 0; selectedRow_ = 0;
        break;

    case KEY_END:
        if (!filtered_.empty()) {
            selectedRow_ = (int)filtered_.size() - 1;
            int vis = ui_.GetVisibleRows();
            scrollOffset_ = std::max(0, (int)filtered_.size() - vis);
        }
        break;

    default: break;
    }
}

void Monitor::Refresh()
{
    store_.Update();
    ApplyFilterAndSort();
    lastRefresh_ = std::chrono::steady_clock::now();

    RenderState rs;
    rs.entries         = &filtered_;
    rs.scrollOffset    = scrollOffset_;
    rs.selectedRow     = selectedRow_;
    rs.okCount         = store_.CountByStatus(DataStatus::OK);
    rs.warnCount       = store_.CountByStatus(DataStatus::WARN);
    rs.errCount        = store_.CountByStatus(DataStatus::ERR);
    rs.refreshInterval = refreshInterval_;
    rs.nextRefreshSec  = GetNextRefreshSec();
    rs.filterLabel     = GetFilterLabel();
    rs.sortLabel       = GetSortLabel();
    rs.paused          = paused_;

    ui_.Render(rs);
}

void Monitor::ApplyFilterAndSort()
{
    const auto& all = store_.GetEntries();
    filtered_.clear();
    filtered_.reserve(all.size());

    for (const auto& e : all) {
        bool keep = true;
        switch (filter_) {
        case FilterMode::OK:   keep = (e.status == DataStatus::OK);   break;
        case FilterMode::WARN: keep = (e.status == DataStatus::WARN); break;
        case FilterMode::ERR:  keep = (e.status == DataStatus::ERR);  break;
        default: break;
        }
        if (keep) filtered_.push_back(e);
    }

    switch (sort_) {
    case SortMode::KEY:
        std::sort(filtered_.begin(), filtered_.end(),
            [](const DataEntry& a, const DataEntry& b) { return a.key < b.key; });
        break;
    case SortMode::STATUS:
        std::sort(filtered_.begin(), filtered_.end(),
            [](const DataEntry& a, const DataEntry& b) { return (int)a.status > (int)b.status; });
        break;
    case SortMode::TYPE:
        std::sort(filtered_.begin(), filtered_.end(),
            [](const DataEntry& a, const DataEntry& b) { return a.typeStr < b.typeStr; });
        break;
    case SortMode::TIME:
        std::sort(filtered_.begin(), filtered_.end(),
            [](const DataEntry& a, const DataEntry& b) { return a.lastUpdated > b.lastUpdated; });
        break;
    }

    if (selectedRow_ >= (int)filtered_.size())
        selectedRow_ = std::max(0, (int)filtered_.size() - 1);
    if (scrollOffset_ > selectedRow_)
        scrollOffset_ = selectedRow_;
}

void Monitor::ScrollUp()
{
    if (selectedRow_ > 0) {
        --selectedRow_;
        if (selectedRow_ < scrollOffset_) scrollOffset_ = selectedRow_;
    }
}

void Monitor::ScrollDown()
{
    if (selectedRow_ < (int)filtered_.size() - 1) {
        ++selectedRow_;
        int vis = ui_.GetVisibleRows();
        if (selectedRow_ >= scrollOffset_ + vis)
            scrollOffset_ = selectedRow_ - vis + 1;
    }
}

void Monitor::PageUp()
{
    int vis = ui_.GetVisibleRows();
    selectedRow_  = std::max(0, selectedRow_  - vis);
    scrollOffset_ = std::max(0, scrollOffset_ - vis);
}

void Monitor::PageDown()
{
    int vis = ui_.GetVisibleRows();
    int sz  = (int)filtered_.size();
    selectedRow_  = std::min(sz - 1, selectedRow_  + vis);
    scrollOffset_ = std::min(std::max(0, sz - vis), scrollOffset_ + vis);
}

std::string Monitor::GetFilterLabel() const
{
    switch (filter_) {
    case FilterMode::ALL:  return "ALL";
    case FilterMode::OK:   return "OK";
    case FilterMode::WARN: return "WARN";
    case FilterMode::ERR:  return "ERR";
    }
    return "ALL";
}

std::string Monitor::GetSortLabel() const
{
    switch (sort_) {
    case SortMode::KEY:    return "KEY";
    case SortMode::STATUS: return "STATUS";
    case SortMode::TYPE:   return "TYPE";
    case SortMode::TIME:   return "TIME";
    }
    return "KEY";
}

int Monitor::GetNextRefreshSec() const
{
    auto now = std::chrono::steady_clock::now();
    auto sec = std::chrono::duration_cast<std::chrono::seconds>(now - lastRefresh_).count();
    int  rem = refreshInterval_ - (int)sec;
    return rem > 0 ? rem : 0;
}
