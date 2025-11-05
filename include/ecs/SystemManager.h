#pragma once

#include "ComponentsManager.h"
#include "IView.h"
#include <memory>
#include <queue>

namespace Cel {
  class SystemManager {
  public:
    explicit SystemManager(const std::shared_ptr<ComponentsManager> &manager)
      : componentsManager(manager) {
    }

    void UpdateViews() const;

    void InitialiseViews();

    static void Queue(const std::shared_ptr<IView> &view);

  private:
    std::shared_ptr<ComponentsManager> componentsManager;
    std::vector<std::shared_ptr<IView> > views;
    static std::queue<std::shared_ptr<IView> > toInitialize;
  };
}
