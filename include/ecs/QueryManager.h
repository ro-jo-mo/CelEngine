#pragma once

#include "ComponentsManager.h"
#include "IQuery.h"
#include <memory>
#include <iostream>

namespace Cel {
  /**
   * @brief Manages the updating of all queries
   * Additionally ensures we don't duplicate queries
   */
  class QueryManager {
  public:
    explicit QueryManager(ComponentsManager &manager)
      : componentsManager(manager) {
    }

    void update_queries() const;

    template<typename Query>
    Query &get_query();

  private:
    ComponentsManager &componentsManager;
    std::unordered_map<std::type_index, std::unique_ptr<IQuery> > queries;
  };

  template<typename Query>
  Query &QueryManager::get_query() {
    auto [query,_] = queries.try_emplace(typeid(Query), std::make_unique<Query>(componentsManager));

    return *static_cast<Query *>(query->second.get());
  }
}


