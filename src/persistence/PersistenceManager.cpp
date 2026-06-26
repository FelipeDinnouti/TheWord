#include "PersistenceManager.h"
#include "core/Logger.h"
#include <sqlite3.h>
#include <sys/stat.h>

namespace theword::persistence {

using namespace theword::core;
using namespace theword::highlight;

PersistenceManager::PersistenceManager(const std::string& dbPath) : db(nullptr) {
    EnsureDirectory(dbPath);
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        Logger::Error("PersistenceManager: Failed to open database: "
                      + std::string(sqlite3_errmsg(db)));
        sqlite3_close(db);
        db = nullptr;
        return;
    }
    InitSchema();
}

PersistenceManager::~PersistenceManager() {
    if (db) sqlite3_close(db);
}

void PersistenceManager::EnsureDirectory(const std::string& dbPath) {
    if (dbPath == ":memory:") return;
    size_t slash = dbPath.rfind('/');
    if (slash == std::string::npos) return;
    std::string dir = dbPath.substr(0, slash);
    mkdir(dir.c_str(), 0755);
}

void PersistenceManager::InitSchema() {
    char* err = nullptr;
    if (sqlite3_exec(db,
        "CREATE TABLE IF NOT EXISTS highlight_types ("
        "  id INTEGER PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  color_r INTEGER NOT NULL,"
        "  color_g INTEGER NOT NULL,"
        "  color_b INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS highlights ("
        "  id INTEGER PRIMARY KEY,"
        "  start_word INTEGER NOT NULL,"
        "  end_word INTEGER NOT NULL,"
        "  type_id INTEGER REFERENCES highlight_types(id)"
        ");"
        "CREATE TABLE IF NOT EXISTS preferences ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ");",
        nullptr, nullptr, &err) != SQLITE_OK) {
        Logger::Error("PersistenceManager: Schema init failed: "
                      + std::string(err));
        sqlite3_free(err);
        return;
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM highlight_types",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Error("PersistenceManager: Failed to count types: "
                      + std::string(sqlite3_errmsg(db)));
        return;
    }
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (count == 0) {
        err = nullptr;
        if (sqlite3_exec(db,
            "INSERT INTO highlight_types (id, name, color_r, color_g, color_b) VALUES "
            "(1, 'Yellow', 255, 235, 59),"
            "(2, 'Pink', 244, 143, 177),"
            "(3, 'Green', 165, 214, 167),"
            "(4, 'Blue', 144, 202, 249),"
            "(5, 'Orange', 255, 204, 128)",
            nullptr, nullptr, &err) != SQLITE_OK) {
            Logger::Error("PersistenceManager: Failed to seed types: "
                          + std::string(err));
            sqlite3_free(err);
        }
    }

    err = nullptr;
    if (sqlite3_exec(db,
        "ALTER TABLE highlights ADD COLUMN provider_name TEXT NOT NULL DEFAULT ''",
        nullptr, nullptr, &err) != SQLITE_OK) {
        sqlite3_free(err);
    }
}

std::vector<Highlight> PersistenceManager::LoadHighlights() {
    std::vector<Highlight> results;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "SELECT id, start_word, end_word, type_id, provider_name FROM highlights",
        -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Error("PersistenceManager: LoadHighlights prepare failed: "
                      + std::string(sqlite3_errmsg(db)));
        return results;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Highlight h;
        h.id = sqlite3_column_int(stmt, 0);
        h.startWord = sqlite3_column_int(stmt, 1);
        h.endWord = sqlite3_column_int(stmt, 2);
        h.typeId = sqlite3_column_int(stmt, 3);
        const char* prov = (const char*)sqlite3_column_text(stmt, 4);
        if (prov) h.providerName = prov;
        results.push_back(h);
    }
    sqlite3_finalize(stmt);
    return results;
}

void PersistenceManager::SaveHighlight(const Highlight& h) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO highlights (id, start_word, end_word, type_id, provider_name) "
        "VALUES (?, ?, ?, ?, ?)",
        -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Error("PersistenceManager: SaveHighlight prepare failed: "
                      + std::string(sqlite3_errmsg(db)));
        return;
    }
    sqlite3_bind_int(stmt, 1, h.id);
    sqlite3_bind_int(stmt, 2, h.startWord);
    sqlite3_bind_int(stmt, 3, h.endWord);
    sqlite3_bind_int(stmt, 4, h.typeId);
    sqlite3_bind_text(stmt, 5, h.providerName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void PersistenceManager::RemoveHighlight(int id) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "DELETE FROM highlights WHERE id = ?",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Error("PersistenceManager: RemoveHighlight prepare failed: "
                      + std::string(sqlite3_errmsg(db)));
        return;
    }
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<HighlightType> PersistenceManager::LoadHighlightTypes() {
    std::vector<HighlightType> results;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "SELECT id, name, color_r, color_g, color_b FROM highlight_types",
        -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Error("PersistenceManager: LoadHighlightTypes prepare failed: "
                      + std::string(sqlite3_errmsg(db)));
        return results;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        HighlightType t;
        t.id = sqlite3_column_int(stmt, 0);
        t.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.color.r = static_cast<unsigned char>(sqlite3_column_int(stmt, 2));
        t.color.g = static_cast<unsigned char>(sqlite3_column_int(stmt, 3));
        t.color.b = static_cast<unsigned char>(sqlite3_column_int(stmt, 4));
        t.color.a = 100;
        results.push_back(t);
    }
    sqlite3_finalize(stmt);
    return results;
}

void PersistenceManager::SaveHighlightType(const HighlightType& t) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO highlight_types (id, name, color_r, color_g, color_b) "
        "VALUES (?, ?, ?, ?, ?)",
        -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::Error("PersistenceManager: SaveHighlightType prepare failed: "
                      + std::string(sqlite3_errmsg(db)));
        return;
    }
    sqlite3_bind_int(stmt, 1, t.id);
    sqlite3_bind_text(stmt, 2, t.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, t.color.r);
    sqlite3_bind_int(stmt, 4, t.color.g);
    sqlite3_bind_int(stmt, 5, t.color.b);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::string PersistenceManager::GetPreference(const std::string& key,
                                               const std::string& defaultValue) const {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT value FROM preferences WHERE key = ?",
                           -1, &stmt, nullptr) != SQLITE_OK) {
        return defaultValue;
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        return val;
    }
    sqlite3_finalize(stmt);
    return defaultValue;
}

void PersistenceManager::SetPreference(const std::string& key,
                                        const std::string& value) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO preferences (key, value) VALUES (?, ?)",
        -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace theword::persistence
