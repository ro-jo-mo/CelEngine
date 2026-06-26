#pragma once
#include "core/Plugin.h"
#include "core/Transform.h"

#include <string>
#include <vector>

namespace KnownValues {
struct TestStruct
{
    std::vector<int> list;
    std::string name;
};

inline TestStruct testStruct{ { 1, 2, 6, 5, 4, 3 }, { "wowee" } };
inline TestStruct testStructOther{ { 1, 2, 3, 4, 5, 6 }, { "nope" } };
inline Cel::Rotation rotation{
    glm::quatLookAtLH<float, glm::packed>({ 0.1, 0.0, 0.0 }, { 0.0, 1.0, 0.0 })
};
}

template<auto Schedule, auto funcPtr>
class SingleSystemPlugin : public Cel::Plugin
{
  public:
    void Build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        scheduler.AddSystem(Schedule, funcPtr);
    }
};