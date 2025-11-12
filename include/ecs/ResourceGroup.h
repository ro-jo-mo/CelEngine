#pragma once
#include "IResourceGroup.h"
#include "ResourceManager.h"

namespace Cel {
    template<typename... Resources>
    class ResourceGroup final : public IResourceGroup {
    public:
        static auto Create();

        void Initialise(ResourceManager &resourceManager) override;

        std::tuple<std::shared_ptr<Resources>...> Get();

    private:
        ResourceGroup() = default;

        std::tuple<std::shared_ptr<Resources>...> resources;
    };

    template<typename... Resources>
    auto ResourceGroup<Resources...>::Create() {
        auto group = std::make_shared<ResourceGroup>();
        ResourceManager::Queue(group);
        return group;
    }

    template<typename... Resources>
    void ResourceGroup<Resources...>::Initialise(ResourceManager &resourceManager) {
        resources = resourceManager.GetResources();
    }

    template<typename... Resources>
    std::tuple<std::shared_ptr<Resources>...> ResourceGroup<Resources...>::Get() {
        return resources;
    }
}

