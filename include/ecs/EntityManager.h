#pragma once

#include "Types.h"
#include <array>
#include <queue>

namespace Cel {
  /**
   * @brief Manages the allocation of entity ids
   * A caveat of the current system is that there is a set maximum number of entity ids available
   * Currently components are stored in arrays, hence the restriction
   * At a later point I might consider switching to vectors depending on performance restrictions
   */
  class EntityManager {
  public:
    /**
     * @brief Returns an unused id
     * @return entity id
     */
    Entity AllocateEntity();

    /**
     * Destroys an entity, and allows the id to be reused
     * @param entity Entity to destroy
     */
    void DestroyEntity(Entity entity);

  private:
    std::queue<Entity> toReuse;
    Entity entityCounter = 0;
  };
}
