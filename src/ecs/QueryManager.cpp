#include "ecs/QueryManager.h"

using namespace Cel;

static std::array<ComponentStorage*, MAX_QUERY_RETRIEVE>
get_retrieved_components(
    const std::array<ComponentId, MAX_QUERY_RETRIEVE>& retrieveOrder,
    const size_t totalRetrieved,
    Archetype& archetype)
{
    std::array<ComponentStorage*, MAX_QUERY_RETRIEVE> storages{};

    for (size_t i = 0; i < totalRetrieved; i++) {
        const auto component = retrieveOrder[i];

        // Entity slots aren't components. Optional components might not
        // exist in this archetype; either way the slot stays nullptr
        if (component < archetype.get_signature().size() &&
            archetype.get_signature().test(component)) {
            storages[i] = &archetype.get_storage(component);
        }
    }

    return storages;
}

void
QueryManager::find_matches_for_new_archetype(Archetype& archetype) const
{
    for (auto& query : queries) {
        // queries is indexed by query id and may have unused slots
        if (query == nullptr) {
            continue;
        }

        if (does_query_match(*query, archetype)) {

            query->matches.push_back(
                { .storages = get_retrieved_components(
                      query->retrieveOrder, query->totalRetrieved, archetype),
                  .archetype = archetype });
        }
    }
}

bool
QueryManager::does_query_match(const IQuery& query, const Archetype& archetype)
{
    // Does this archetype contain entities that match the required
    // components and not the rejected components
    return (query.required & archetype.get_signature()) == query.required &&
           (query.reject & archetype.get_signature()).none();
}

void
QueryManager::initialise_query(IQuery& query) const
{
    for (const auto& archetype : archetypeManager.archetypes) {
        if (does_query_match(query, *archetype)) {
            query.matches.push_back(
                { .storages = get_retrieved_components(
                      query.retrieveOrder, query.totalRetrieved, *archetype),
                  .archetype = *archetype });
        }
    }
}
