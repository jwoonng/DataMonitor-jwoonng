#include "DataStore.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Helpers to build initial entries
// ---------------------------------------------------------------------------
static DataEntry MakeNumeric(int id, const char* key, const char* type,
    double val, double maxVal, double warn, double err,
    bool invert, const char* unit, const char* desc)
{
    DataEntry e{};
    e.id           = id;
    e.key          = key;
    e.typeStr      = type;
    e.description  = desc;
    e.isNumeric    = true;
    e.numericVal   = val;
    e.maxVal       = maxVal;
    e.warnThresh   = warn;
    e.errThresh    = err;
    e.invertThresh = invert;
    e.unit         = unit;
    e.lastUpdated  = std::time(nullptr);
    return e;
}

static DataEntry MakeString(int id, const char* key, const char* val, const char* desc)
{
    DataEntry e{};
    e.id          = id;
    e.key         = key;
    e.typeStr     = "CONFIG";
    e.value       = val;
    e.description = desc;
    e.isNumeric   = false;
    e.status      = DataStatus::OK;
    e.lastUpdated = std::time(nullptr);
    return e;
}

// ---------------------------------------------------------------------------
// DataStore
// ---------------------------------------------------------------------------
DataStore::DataStore()
    : rng_(std::random_device{}())
{
    InitEntries();
    for (auto& e : entries_) {
        if (e.isNumeric) { RefreshValue(e); UpdateStatus(e); }
    }
}

void DataStore::InitEntries()
{
    int n = 1;
    //                      id  key                         type      init   max   warn  err   inv  unit    desc
    entries_.push_back(MakeNumeric(n++,"db.connections.active",  "GAUGE",   15,  100,   70,  90, false,"",    "Active DB connections"));
    entries_.push_back(MakeNumeric(n++,"db.connections.idle",    "GAUGE",   20,  100,    5,   2, true, "",    "Idle DB connections"));
    entries_.push_back(MakeNumeric(n++,"db.query.avg_ms",        "METRIC",  45, 5000,  500,1000, false,"ms",  "Avg DB query latency"));
    entries_.push_back(MakeNumeric(n++,"cache.hit_rate",         "METRIC",  87,  100,   70,  50, true, "%",   "Cache hit rate"));
    entries_.push_back(MakeNumeric(n++,"cache.size_mb",          "GAUGE",  512, 2048, 1600,1900, false,"MB",  "Cache memory usage"));
    entries_.push_back(MakeNumeric(n++,"cache.evictions_per_min","COUNTER",  5, 1000,  100, 300, false,"",    "Cache evictions/min"));
    entries_.push_back(MakeNumeric(n++,"queue.msg.depth",        "GAUGE",  250, 5000, 1000,3000, false,"",    "Message queue depth"));
    entries_.push_back(MakeNumeric(n++,"queue.consumer.lag",     "GAUGE",   12, 1000,  100, 500, false,"msg", "Consumer lag"));
    entries_.push_back(MakeNumeric(n++,"session.active_count",   "GAUGE",  150, 1000,  700, 900, false,"",    "Active sessions"));
    entries_.push_back(MakeNumeric(n++,"sys.cpu.usage",          "METRIC",  35,  100,   75,  90, false,"%",   "CPU utilization"));
    entries_.push_back(MakeNumeric(n++,"sys.mem.usage",          "METRIC",  62,  100,   80,  95, false,"%",   "Memory utilization"));
    entries_.push_back(MakeNumeric(n++,"sys.disk.io_mb_s",       "METRIC",  45, 1000,  400, 700, false,"MB/s","Disk I/O throughput"));
    entries_.push_back(MakeNumeric(n++,"net.bandwidth.usage",    "METRIC",  28,  100,   75,  90, false,"%",   "Network bandwidth"));
    entries_.push_back(MakeNumeric(n++,"api.response_time_ms",   "METRIC", 120,10000,  500,1000, false,"ms",  "API avg response time"));
    entries_.push_back(MakeNumeric(n++,"api.error_rate",         "METRIC",  0.3, 100,    1,   5, false,"%",   "API error rate"));
    entries_.push_back(MakeNumeric(n++,"api.req_per_sec",        "COUNTER",320, 2000, 1500,1800, false,"",    "API requests/sec"));
    entries_.push_back(MakeNumeric(n++,"jobs.pending",           "GAUGE",    8,  200,   50, 100, false,"",    "Background jobs pending"));
    entries_.push_back(MakeNumeric(n++,"jobs.failed_last_hour",  "COUNTER",  2, 1000,    5,  20, false,"",    "Failed jobs (last hr)"));
    entries_.push_back(MakeNumeric(n++,"replication.lag_ms",     "METRIC",  15,  500,   50, 200, false,"ms",  "DB replication lag"));
    entries_.push_back(MakeNumeric(n++,"thread_pool.active",     "GAUGE",   12,   32,   24,  30, false,"",    "Thread pool active threads"));
    entries_.push_back(MakeString (n++,"app.version",            "2.4.1",             "Application version"));
    entries_.push_back(MakeString (n++,"app.environment",        "production",        "Deployment environment"));
    entries_.push_back(MakeString (n++,"app.debug_mode",         "false",             "Debug mode enabled"));
    entries_.push_back(MakeString (n++,"db.pool.max_size",       "100",               "DB connection pool max"));
}

void DataStore::Update()
{
    ++tick_;
    for (auto& e : entries_)
        if (e.isNumeric) SimulateStep(e);
}

void DataStore::SimulateStep(DataEntry& e)
{
    std::uniform_real_distribution<double> noise(-1.0, 1.0);
    std::uniform_int_distribution<int>     spike(0, 120);

    // Per-metric volatility scale
    double scale = 1.5;
    if      (e.key == "queue.msg.depth")       scale = 45.0;
    else if (e.key == "api.req_per_sec")       scale = 25.0;
    else if (e.key == "sys.cpu.usage")         scale = 4.0;
    else if (e.key == "api.response_time_ms")  scale = 18.0;
    else if (e.key == "cache.hit_rate")        scale = 0.4;
    else if (e.key == "sys.mem.usage")         scale = 0.6;
    else if (e.key == "replication.lag_ms")    scale = 6.0;
    else if (e.key == "api.error_rate")        scale = 0.12;

    e.numericVal += noise(rng_) * scale;

    // Occasional spike to exercise threshold coloring
    if (spike(rng_) == 0) {
        if (e.invertThresh)
            e.numericVal = e.errThresh * 0.8;
        else
            e.numericVal = e.errThresh * 1.05;
    }

    double lo = e.invertThresh ? e.errThresh * 0.5 : 0.0;
    if (e.numericVal < lo)        e.numericVal = lo;
    if (e.numericVal > e.maxVal)  e.numericVal = e.maxVal;

    RefreshValue(e);
    UpdateStatus(e);
    e.lastUpdated = std::time(nullptr);
}

void DataStore::RefreshValue(DataEntry& e)
{
    std::ostringstream oss;
    oss << std::fixed;

    const std::string& u = e.unit;
    if (u == "%" || u.empty() && (e.key.find("rate") != std::string::npos || e.key.find("usage") != std::string::npos))
        oss << std::setprecision(1) << e.numericVal << (u.empty() ? "" : " " + u);
    else if (u == "ms")
        oss << std::setprecision(0) << e.numericVal << " ms";
    else if (u == "MB" || u == "MB/s")
        oss << std::setprecision(0) << e.numericVal << " " << u;
    else {
        oss << std::setprecision(0) << e.numericVal;
        if (!u.empty()) oss << " " << u;
    }

    e.value = oss.str();
}

void DataStore::UpdateStatus(DataEntry& e)
{
    if (!e.isNumeric) return;
    if (e.invertThresh) {
        if      (e.numericVal <= e.errThresh)  e.status = DataStatus::ERR;
        else if (e.numericVal <= e.warnThresh) e.status = DataStatus::WARN;
        else                                    e.status = DataStatus::OK;
    } else {
        if      (e.numericVal >= e.errThresh)  e.status = DataStatus::ERR;
        else if (e.numericVal >= e.warnThresh) e.status = DataStatus::WARN;
        else                                    e.status = DataStatus::OK;
    }
}

const std::vector<DataEntry>& DataStore::GetEntries() const { return entries_; }

int DataStore::CountByStatus(DataStatus s) const
{
    int cnt = 0;
    for (const auto& e : entries_)
        if (e.status == s) ++cnt;
    return cnt;
}
