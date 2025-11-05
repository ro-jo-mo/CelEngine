#pragma once

#include "ComponentsManager.h"
#include <memory>

namespace Cel {
    class IView {
    public:
        virtual ~IView() = default;

        virtual void Initialise(std::shared_ptr<Cel::ComponentsManager> manager) = 0;

        virtual void UpdateView() = 0;
    };
}
