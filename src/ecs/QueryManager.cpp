#include "ecs/QueryManager.h"

using namespace Cel;

static std::array<ComponentStorage*, MAX_QUERY_RETRIEVE>
get_required_components(const Signature& signature, Archetype& archetype)
{
    std::array<ComponentStorage*, MAX_QUERY_RETRIEVE> storages{};

    size_t currentIndex = 0;

    for (size_t i = 0; i < MAX_COMPONENTS; i++) {
        if (signature.test(i)) {

            // Check for optional component
            if (archetype.get_signature().test(i)) {
                storages[currentIndex] = &archetype.get_storage(i);
            }

            currentIndex++;
        }
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

            query->matches.push_back({ .storages = get_required_components(
                                           query->required, archetype),
                                       .archetype = archetype });
        }
    }
}
