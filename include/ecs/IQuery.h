#pragma once

#include "ComponentsManager.h"
#include <memory>

namespace Cel {
struct ComponentStorage;
class Archetype;

struct QueryMatch
{
    // Query marks 0..n indices. If indice x is nullptr, means it was
    // optional and not available
    std::array<ComponentStorage*, MAX_QUERY_RETRIEVE> storages;
    // Store a ptr to the archetype. Used to get total entities and entity
    // id from index
    Archetype& archetype;
};

/**
 * @brief Dummy interface for query polymorphism
 */
class IQuery
{
  public:
    virtual ~IQuery() = default;

  protected:
    // A list of archetypes that match this
    std::vector<QueryMatch> matches;

    // The ordering the query uses when retrieving,
    // matching Get<A,B,C> to { A.id, B.id, C.id }
    std::array<ComponentId, MAX_QUERY_RETRIEVE> retrieveOrder;
    size_t totalRetrieved;

    Signature retrieve{};
    Signature required{};
    Signature reject{};

    friend class QueryManager;
};
}
