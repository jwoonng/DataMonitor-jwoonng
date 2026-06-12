#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <string>
#include <vector>
#include <ctime>
#include <random>

enum class DataStatus { OK, WARN, ERR };

struct DataEntry {
    int         id;
    std::string key;
    std::string value;
    std::string typeStr;
    DataStatus  status;
    time_t      lastUpdated;
    std::string description;

    // numeric simulation fields
    bool   isNumeric    = false;
    double numericVal   = 0.0;
    double maxVal       = 100.0;
    double warnThresh   = 70.0;
    double errThresh    = 90.0;
    bool   invertThresh = false; // true = lower is worse (e.g. hit rate)
    std::string unit;
};

class DataStore {
public:
    DataStore();
    void Update();
    const std::vector<DataEntry>& GetEntries() const;
    int CountByStatus(DataStatus s) const;

private:
    std::vector<DataEntry> entries_;
    std::mt19937           rng_;
    int                    tick_ = 0;

    void InitEntries();
    void SimulateStep(DataEntry& e);
    void RefreshValue(DataEntry& e);
    void UpdateStatus(DataEntry& e);
};
