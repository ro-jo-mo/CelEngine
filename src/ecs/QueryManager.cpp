#include "ecs/QueryManager.h"

using namespace Cel;

static std::array<ComponentStorage*, MAX_QUERY_RETRIEVE>
get_retrieved_components(
    const std::array<ComponentId, MAX_QUERY_RETRIEVE>& retrieveOrder,
    Archetype& archetype)
{
    std::array<ComponentStorage*, MAX_QUERY_RETRIEVE> storages{};

    size_t currentIndex = 0;

    for (const auto component : retrieveOrder) {

        // Optional components might not exist in this archetype
        // We still increment the counter
        if (archetype.get_signature().test(component)) {
            storages[currentIndex] = &archetype.get_storage(component);
        }

        currentIndex++;
    }

    return storages;
}

void
QueryManager::find_matches_for_new_archetype(Archetype& archetype) const
{
    for (auto& query : queries) {
        // Does this archetype contain entities that match the required
        // components and not the rejected components
        if ((query->required & archetype.get_signature()) == query->required &&
            (query->reject & archetype.get_signature()).none()) {

            query->matches.push_back({ .storages = get_retrieved_components(
                                           query->retrieveOrder, archetype),
                                       .archetype = archetype });
        }
    }
}
