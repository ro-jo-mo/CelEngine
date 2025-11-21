#pragma once

#include "ComponentsManager.h"
#include "EntityManager.h"
#include "Types.h"
#include <vector>

#include "Resource.h"

namespace Cel {
  class World : public Resource {
  public:
    World(const std::shared_ptr<ComponentsManager> &componentMngr, EntityManager &entityMngr) : componentsManager(
        componentMngr),
      entityManager(entityMngr) {
    }

    template<typename... Components>
    Entity Spawn(Components... components);

    void Destroy(Entity entity);

    template<typename Component>
    void AddComponent(Entity entity, Component component);

    template<typename Component>
    void RemoveComponent(Entity entity);

    bool Flush();

  private:
    class Command {
    public:
      virtual ~Command() = default;

      virtual void Execute() = 0;
    };

    template<typename T>
    class AddCommand;

    template<typename T>
    class RemoveCommand;

    void ExecuteDestroy(Entity entity) const;

    std::shared_ptr<ComponentsManager> componentsManager;
    EntityManager &entityManager;
    std::vector<std::unique_ptr<Command> > toAdd;
    std::vector<std::unique_ptr<Command> > toRemove;
    std::vector<Entity> toDestroy;
  };

  template<typename T>
  class World::AddCommand final : public World::Command {
  public:
    AddCommand(const Entity ent, T comp, World &wld)
      : entity(ent)
        , component(comp)
        , world(wld) {
    };

    void Execute() override;

  private:
    Entity entity;
    T component;
    World &world;
  };

  template<typename T>
  class World::RemoveCommand final : public World::Command {
  public:
    RemoveCommand(const Entity ent, World &wld)
      : entity(ent)
        , world(wld) {
    };

    void Execute() override;

  private:
    Entity entity;
    World &world;
  };

  template<typename... Components>
  inline Entity
  World::Spawn(Components... components) {
    auto id = entityManager.AllocateEntity();
    ((void) AddComponent(id, components), ...);
    return id;
  }

  template<typename Component>
  inline void
  World::AddComponent(Entity entity, Component component) {
    toAdd.push_back(
      std::make_unique<AddCommand<Component> >(entity, component, *this));
  }

  template<typename Component>
  inline void
  World::RemoveComponent(Entity entity) {
    toRemove.push_back(std::make_unique<RemoveCommand<Component> >(entity, *this));
  }

  template<typename T>
  inline void
  World::AddCommand<T>::Execute() {
    world.componentsManager->AddComponent(entity, component);
  }

  template<typename T>
  inline void
  World::RemoveCommand<T>::Execute() {
    world.componentsManager->RemoveComponent<T>(entity);
  }
}
