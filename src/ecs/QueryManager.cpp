#include "ecs/QueryManager.h"

#include <ranges>

using namespace Cel;

void QueryManager::UpdateQueries() const {
  for (const auto &query: queries | std::views::values) {
    query->UpdateQuery();
  }
}

