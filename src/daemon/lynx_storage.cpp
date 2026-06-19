#include "lynx_storage.h"

#include <sqlite3.h>
#include <cstdio>
#include <cstring>

LynxStorage::LynxStorage(const std::string &db_path) {
    sqlite3 *db = NULL;
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    int rc = sqlite3_open_v2(db_path.c_str(), &db, flags, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[lynx_storage] open failed: %s\n", sqlite3_errmsg(db));
        db_ = NULL;
        return;
    }
    db_ = db;

    exec("PRAGMA journal_mode=WAL");
    exec("PRAGMA synchronous=NORMAL");

    const char *sql =
        "CREATE TABLE IF NOT EXISTS metrics ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    node_id TEXT NOT NULL,"
        "    timestamp_ns INTEGER NOT NULL,"
        "    cpu_user INTEGER NOT NULL,"
        "    cpu_system INTEGER NOT NULL,"
        "    cpu_idle INTEGER NOT NULL,"
        "    mem_total_kb INTEGER NOT NULL,"
        "    mem_avail_kb INTEGER NOT NULL,"
        "    load_1m REAL NOT NULL,"
        "    load_5m REAL NOT NULL,"
        "    load_15m REAL NOT NULL"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_metrics_node_ts ON metrics(node_id, timestamp_ns);";
    exec(sql);
}

LynxStorage::~LynxStorage() {
    if (db_) {
        sqlite3_close(static_cast<sqlite3 *>(db_));
    }
}

int LynxStorage::exec(const char *sql) {
    if (!db_) return -1;
    char *err = NULL;
    int rc = sqlite3_exec(static_cast<sqlite3 *>(db_), sql, NULL, NULL, &err);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[lynx_storage] exec error: %s\n", err);
        sqlite3_free(err);
        return -1;
    }
    return 0;
}

int LynxStorage::store_metrics(const std::string &node_id, uint64_t timestamp_ns,
                               uint64_t cpu_user, uint64_t cpu_system, uint64_t cpu_idle,
                               uint64_t mem_total_kb, uint64_t mem_avail_kb,
                               double load_1m, double load_5m, double load_15m) {
    if (!db_) return -1;
    sqlite3 *db = static_cast<sqlite3 *>(db_);
    const char *sql =
        "INSERT INTO metrics (node_id, timestamp_ns, cpu_user, cpu_system, cpu_idle, "
        "mem_total_kb, mem_avail_kb, load_1m, load_5m, load_15m) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[lynx_storage] prepare failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    sqlite3_bind_text(stmt, 1, node_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, (sqlite3_int64)timestamp_ns);
    sqlite3_bind_int64(stmt, 3, (sqlite3_int64)cpu_user);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)cpu_system);
    sqlite3_bind_int64(stmt, 5, (sqlite3_int64)cpu_idle);
    sqlite3_bind_int64(stmt, 6, (sqlite3_int64)mem_total_kb);
    sqlite3_bind_int64(stmt, 7, (sqlite3_int64)mem_avail_kb);
    sqlite3_bind_double(stmt, 8, load_1m);
    sqlite3_bind_double(stmt, 9, load_5m);
    sqlite3_bind_double(stmt, 10, load_15m);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        fprintf(stderr, "[lynx_storage] step failed: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    return 0;
}

std::vector<StoredMetrics> LynxStorage::query_recent(const std::string &node_id, int limit) {
    std::vector<StoredMetrics> results;
    if (!db_) return results;
    sqlite3 *db = static_cast<sqlite3 *>(db_);
    const char *sql =
        "SELECT id, node_id, timestamp_ns, cpu_user, cpu_system, cpu_idle, "
        "mem_total_kb, mem_avail_kb, load_1m, load_5m, load_15m "
        "FROM metrics WHERE node_id=? ORDER BY timestamp_ns DESC LIMIT ?";
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "[lynx_storage] query prepare failed: %s\n", sqlite3_errmsg(db));
        return results;
    }

    sqlite3_bind_text(stmt, 1, node_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, limit);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        StoredMetrics m;
        m.id = sqlite3_column_int64(stmt, 0);
        const char *nid = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 1));
        m.node_id = nid ? nid : "";
        m.timestamp_ns = (uint64_t)sqlite3_column_int64(stmt, 2);
        m.cpu_user = (uint64_t)sqlite3_column_int64(stmt, 3);
        m.cpu_system = (uint64_t)sqlite3_column_int64(stmt, 4);
        m.cpu_idle = (uint64_t)sqlite3_column_int64(stmt, 5);
        m.mem_total_kb = (uint64_t)sqlite3_column_int64(stmt, 6);
        m.mem_avail_kb = (uint64_t)sqlite3_column_int64(stmt, 7);
        m.load_1m = sqlite3_column_double(stmt, 8);
        m.load_5m = sqlite3_column_double(stmt, 9);
        m.load_15m = sqlite3_column_double(stmt, 10);
        results.push_back(m);
    }

    sqlite3_finalize(stmt);
    return results;
}
