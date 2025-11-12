#pragma once

namespace Cel {
    class ResourceManager;

    class IResourceGroup {
    public:
        virtual ~IResourceGroup() = default;

        virtual void Initialise(ResourceManager &resourceManager) = 0;
    };
}
