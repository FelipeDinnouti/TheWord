#ifndef PERSISTENCE_MANAGER_H
#define PERSISTENCE_MANAGER_H

#include "highlight/PersistenceInterface.h"
#include <string>

struct sqlite3;

class PersistenceManager : public PersistenceInterface {
public:
    explicit PersistenceManager(const std::string& dbPath);
    ~PersistenceManager() override;
    PersistenceManager(const PersistenceManager&) = delete;
    PersistenceManager& operator=(const PersistenceManager&) = delete;
    PersistenceManager(PersistenceManager&&) = delete;
    PersistenceManager& operator=(PersistenceManager&&) = delete;

    std::vector<Highlight> loadHighlights() override;
    void saveHighlight(const Highlight& h) override;
    void removeHighlight(int id) override;

    std::vector<HighlightType> loadHighlightTypes();
    void saveHighlightType(const HighlightType& t);

    std::string getPreference(const std::string& key, const std::string& defaultValue);
    void setPreference(const std::string& key, const std::string& value);

private:
    sqlite3* db;
    void initSchema();
    void ensureDirectory(const std::string& dbPath);
};

#endif
