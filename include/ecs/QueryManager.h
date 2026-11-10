#pragma once

#include "ArchetypeManager.h"
#include "ComponentsManager.h"
#include "IQuery.h"
#include <iostream>
#include <memory>

namespace Cel {
class Archetype;
/**
 * @brief Manages the updating of all queries
 * Additionally ensures we don't duplicate queries¦
 */
class QueryManager
{
  public:
    explicit QueryManager(ArchetypeManager& archetypeManager)
        : archetypeManager(archetypeManager)
    {
    }

    template<typename Query>
    Query& get_query();

    // When creating a new archetype, we find all queries that accept it
    void find_matches_for_new_archetype(Archetype archetype);

  private:
    ArchetypeManager& archetypeManager;
    std::vector<std::unique_ptr<IQuery>> queries;
};

template<typename Query>
Query&
QueryManager::get_query()
{
    auto id = query_id<Query>();

    if (queries.size() <= id) {
        queries.resize(id + 32);
    }

    if (queries[id] == nullptr) {
        // For now, we make a big assumption that no archetypes exists at query registration.
        // This will *probably* hold true forever. If queries are ever added dynamically throughout the runtime, this will be false
        queries[id] = std::make_unique<Query>();
    }

    return *static_cast<Query*>(queries[id]);
}
}
