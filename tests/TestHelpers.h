#pragma once
#include "core/Plugin.h"
#include "core/Running.h"
#include "core/Transform.h"

#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Shared test components
// ---------------------------------------------------------------------------

struct Health
{
    int value = 100;
};

struct Velocity
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct Tag
{};

struct Disabled
{};

struct Counter
{
    int count = 0;
};

struct IntResource
{
    int value = 0;
};

struct StringResource
{
    std::string value;
};

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
    void build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        scheduler.add_system(Schedule, funcPtr);
    }
};

inline void
StopRunning(Cel::Resource<Cel::Running>& running)
{
    running->isRunning = false;
}

template<auto Schedule>
class StopAfterOneFramePlugin : public Cel::Plugin
{
  public:
    void build(Cel::Scheduler scheduler, Cel::ResourceManager&) override
    {
        scheduler.add_system(Schedule, StopRunning);
    }
};

template<auto Schedule, auto... Systems>
class ChainPlugin : public Cel::Plugin
{
  public:
    void build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        scheduler.add_chain(Schedule, Systems...);
    }
};

template<auto Schedule, auto... Systems>
class GroupPlugin : public Cel::Plugin
{
  public:
    void build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        scheduler.add_group(Schedule, Systems...);
    }
};

template<auto Schedule, auto First, auto Second>
class BeforePlugin : public Cel::Plugin
{
  public:
    void build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        scheduler.add_system(Schedule, First).before(Second);
    }
};

template<auto Schedule, auto First, auto Second>
class AfterPlugin : public Cel::Plugin
{
  public:
    void build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        scheduler.add_system(Schedule, Second).after(First);
    }
};

template<typename ResType, auto... Args>
class ResourcePlugin : public Cel::Plugin
{
  public:
    void build(Cel::Scheduler scheduler,
               Cel::ResourceManager& resourceManager) override
    {
        resourceManager.insert_resource(ResType{ Args... });
    }
};