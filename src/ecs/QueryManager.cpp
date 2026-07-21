#include "ecs/QueryManager.h"

#include <ranges>

using namespace Cel;

void QueryManager::update_queries() const {
  for (const auto &query: queries | std::views::values) {
    query->update_query();
  }
}

