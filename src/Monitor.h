#pragma once
#include "DataStore.h"
#include "ConsoleUI.h"
#include <atomic>
#include <chrono>
#include <vector>
#include <string>

enum class FilterMode { ALL, OK, WARN, ERR };
enum class SortMode   { KEY, STATUS, TYPE, TIME };

class Monitor {
public:
    explicit Monitor(std::atomic<bool>& running);
    void Run();

private:
    std::atomic<bool>& running_;

    DataStore  store_;
    ConsoleUI  ui_;

    bool       paused_           = false;
    int        refreshInterval_  = 2;
    FilterMode filter_           = FilterMode::ALL;
    SortMode   sort_             = SortMode::KEY;
    int        scrollOffset_     = 0;
    int        selectedRow_      = 0;

    std::vector<DataEntry> filtered_;
    std::chrono::steady_clock::time_point lastRefresh_;

    void HandleInput(int ch);
    void Refresh();
    void ApplyFilterAndSort();
    void ScrollUp();
    void ScrollDown();
    void PageUp();
    void PageDown();

    std::string GetFilterLabel() const;
    std::string GetSortLabel() const;
    int         GetNextRefreshSec() const;
};
