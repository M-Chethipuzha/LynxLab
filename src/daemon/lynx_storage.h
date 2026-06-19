#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct StoredMetrics {
    int64_t id;
    std::string node_id;
    uint64_t timestamp_ns;
    uint64_t cpu_user;
    uint64_t cpu_system;
    uint64_t cpu_idle;
    uint64_t mem_total_kb;
    uint64_t mem_avail_kb;
    double load_1m;
    double load_5m;
    double load_15m;
};

class LynxStorage {
public:
    explicit LynxStorage(const std::string &db_path);
    ~LynxStorage();

    LynxStorage(const LynxStorage &) = delete;
    LynxStorage &operator=(const LynxStorage &) = delete;

    int store_metrics(const std::string &node_id, uint64_t timestamp_ns,
                      uint64_t cpu_user, uint64_t cpu_system, uint64_t cpu_idle,
                      uint64_t mem_total_kb, uint64_t mem_avail_kb,
                      double load_1m, double load_5m, double load_15m);

    std::vector<StoredMetrics> query_recent(const std::string &node_id, int limit = 10);

private:
    void *db_;
    int exec(const char *sql);
};
