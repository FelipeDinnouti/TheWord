#include "PersistenceManager.h"
#include <sqlite3.h>
#include <cstdlib>
#include <sys/stat.h>

PersistenceManager::PersistenceManager(const std::string& dbPath) : db(nullptr) {
    ensureDirectory(dbPath);
    sqlite3_open(dbPath.c_str(), &db);
    initSchema();
}

PersistenceManager::~PersistenceManager() {
    if (db) sqlite3_close(db);
}

void PersistenceManager::ensureDirectory(const std::string& dbPath) {
    if (dbPath == ":memory:") return;
    size_t slash = dbPath.rfind('/');
    if (slash == std::string::npos) return;
    std::string dir = dbPath.substr(0, slash);
    mkdir(dir.c_str(), 0755);
}

void PersistenceManager::initSchema() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS highlight_types (
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            color_r INTEGER NOT NULL,
            color_g INTEGER NOT NULL,
            color_b INTEGER NOT NULL
        );
        CREATE TABLE IF NOT EXISTS highlights (
            id INTEGER PRIMARY KEY,
            start_word INTEGER NOT NULL,
            end_word INTEGER NOT NULL,
            type_id INTEGER REFERENCES highlight_types(id)
        );
        CREATE TABLE IF NOT EXISTS preferences (
            key TEXT PRIMARY KEY,
            value TEXT NOT NULL
        );
    )";
    char* err = nullptr;
    sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (err) {
        sqlite3_free(err);
        return;
    }

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM highlight_types", -1, &stmt, nullptr);
    sqlite3_step(stmt);
    int count = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (count == 0) {
        sqlite3_exec(db,
            "INSERT INTO highlight_types (id, name, color_r, color_g, color_b) "
            "VALUES (1, 'Yellow', 255, 255, 0)",
            nullptr, nullptr, nullptr);
    }
}

std::vector<Highlight> PersistenceManager::loadHighlights() {
    std::vector<Highlight> results;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id, start_word, end_word, type_id FROM highlights", -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Highlight h;
        h.id = sqlite3_column_int(stmt, 0);
        h.startWord = sqlite3_column_int(stmt, 1);
        h.endWord = sqlite3_column_int(stmt, 2);
        h.typeId = sqlite3_column_int(stmt, 3);
        results.push_back(h);
    }
    sqlite3_finalize(stmt);
    return results;
}

void PersistenceManager::saveHighlight(const Highlight& h) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT INTO highlights (id, start_word, end_word, type_id) VALUES (?, ?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, h.id);
    sqlite3_bind_int(stmt, 2, h.startWord);
    sqlite3_bind_int(stmt, 3, h.endWord);
    sqlite3_bind_int(stmt, 4, h.typeId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void PersistenceManager::removeHighlight(int id) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "DELETE FROM highlights WHERE id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::vector<HighlightType> PersistenceManager::loadHighlightTypes() {
    std::vector<HighlightType> results;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT id, name, color_r, color_g, color_b FROM highlight_types", -1, &stmt, nullptr);
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

void PersistenceManager::saveHighlightType(const HighlightType& t) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO highlight_types (id, name, color_r, color_g, color_b) VALUES (?, ?, ?, ?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, t.id);
    sqlite3_bind_text(stmt, 2, t.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, t.color.r);
    sqlite3_bind_int(stmt, 4, t.color.g);
    sqlite3_bind_int(stmt, 5, t.color.b);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::string PersistenceManager::getPreference(const std::string& key, const std::string& defaultValue) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT value FROM preferences WHERE key = ?", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        return val;
    }
    sqlite3_finalize(stmt);
    return defaultValue;
}

void PersistenceManager::setPreference(const std::string& key, const std::string& value) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO preferences (key, value) VALUES (?, ?)",
        -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
