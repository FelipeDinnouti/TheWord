#ifndef PERSISTENCE_MANAGER_H
#define PERSISTENCE_MANAGER_H

#include "highlight/PersistenceInterface.h"
#include <string>

struct sqlite3; // opaque pointer matches sqlite3 typedef in sqlite3.h

namespace theword::persistence {

class PersistenceManager : public theword::highlight::PersistenceInterface {
public:
    explicit PersistenceManager(const std::string& dbPath);
    ~PersistenceManager() override;
    PersistenceManager(const PersistenceManager&) = delete;
    PersistenceManager& operator=(const PersistenceManager&) = delete;
    PersistenceManager(PersistenceManager&&) = delete;
    PersistenceManager& operator=(PersistenceManager&&) = delete;

    std::vector<theword::highlight::Highlight> LoadHighlights() override;
    void SaveHighlight(const theword::highlight::Highlight& h) override;
    void RemoveHighlight(int id) override;
    std::vector<theword::highlight::HighlightType> LoadHighlightTypes() override;
    void SaveHighlightType(const theword::highlight::HighlightType& t);

    std::string GetPreference(const std::string& key, const std::string& defaultValue) const;
    void SetPreference(const std::string& key, const std::string& value);

private:
    sqlite3* db;
    void InitSchema();
    void EnsureDirectory(const std::string& dbPath);
};

} // namespace theword::persistence

#endif
