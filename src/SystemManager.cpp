#include "ecs/SystemManager.h"

using namespace Cel;

void
SystemManager::UpdateViews() const {
  for (const auto &view: views) {
    view->UpdateView();
  }
}

void SystemManager::InitialiseViews() {
  while (!toInitialise.empty()) {
    auto &view = toInitialise.front();
    toInitialise.pop();
    view->Initialise(componentsManager);
    views.push_back(view);
  }
}

void SystemManager::Queue(const std::shared_ptr<IView> &view) {
  toInitialise.push(view);
}
