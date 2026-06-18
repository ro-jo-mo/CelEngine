#pragma once

#include "Helpers.h"
#include "QueryManager.h"
#include "ResourceManager.h"

#include <functional>

namespace Cel {

// This is a collection of type helpers for functions

template<typename... Parameters>
struct ParameterList
{};

template<typename ReturnT, typename... ParameterTs>
struct FunctionType
{
    using Parameters = ParameterList<ParameterTs...>;
};

// For reference:
// Function pointer type of a typical function
// i.e. int main(float x,int y,...);
// Has type: int (*pointer)(float,int,...);
// Removing variable name "pointer" results in type: int (*)(float,int,...);
template<typename ReturnT, typename... ParameterTs>
FunctionType<ReturnT, ParameterTs...>
GetFunctionType(ReturnT (*)(ParameterTs...))
{
    return FunctionType<ReturnT, ParameterTs...>{};
}

// Type of a class member function:
// int MyClass::MyFunc(float x, int y,...);
// Has type: int (MyClass::*)(float,int,...);
// For now, I don't think it makes much sense to create systems from a member
// function, given we don't have a class instance
// To my knowledge static functions work fine with the above

// template<typename ClassT, typename ReturnT, typename...
// ParameterTs> FunctionType<ReturnT, ParameterTs...> GetFunctionType(
//     ReturnT (ClassT::*)(ParameterTs...));

// Lambda types are implementation defined?
// Instead we get their type by passing the actual calling function instead
// Similar to class functions, I am not sure there's significant value in
// accepting lambdas

// template<typename T> auto GetFunctionType(T) ->
// decltype(GetFunctionType(&T::operator()));

/**
 * @brief Allocates resources and queries to systems.
 */
class SystemAllocator
{
  public:
    SystemAllocator(ResourceManager& resource_manager,
                    QueryManager& query_manager)
        : resourceManager(resource_manager)
        , queryManager(query_manager)
    {
    }

    template<typename System>
    std::function<void()> AllocateSystem(System system);

  private:
    /**
     * @brief Registers a resource / query to the system, and returns the actual
     * data.
     * @tparam T Resource / query type
     * @return The actual Resource / query
     */
    template<typename T>
    T Register();

    template<typename T>
    struct RegisterAll;

    std::vector<std::type_index> registeredResources;
    std::vector<std::type_index> registeredQueries;
    ResourceManager& resourceManager;
    QueryManager& queryManager;
};

// Create a simple wrapper class to allow partial template specialisation
template<typename... Parameters>
struct SystemAllocator::RegisterAll<ParameterList<Parameters...>>
{
    static std::tuple<Parameters&...> Execute(SystemAllocator& allocator);
};

template<typename... Parameters>
std::tuple<Parameters&...>
SystemAllocator::RegisterAll<ParameterList<Parameters...>>::Execute(
    SystemAllocator& allocator)
{
    return std::tie(allocator.Register<Parameters>()...);
}

template<typename System>
std::function<void()>
SystemAllocator::AllocateSystem(System system)
{
    // ParameterList<Params...>
    using Parameters = decltype(GetFunctionType(system))::Parameters;

    auto args = RegisterAll<Parameters>::Execute(*this);

    return [=, args = std::move(args)]() { std::apply(system, args); };
}

template<typename _T>
_T
SystemAllocator::Register()
{
    using T = std::remove_reference_t<_T>;

    if constexpr (IsQuery<T>::value) {
        registeredQueries.push_back(std::type_index(typeid(T)));
        return queryManager.GetQuery<T>();
    } else if constexpr (IsResource<T>::value) {
        registeredResources.push_back(std::type_index(typeid(T)));
        return resourceManager.GetResource<typename T::inner>();
    } else {
        static_assert(alwaysFalse<T>,
                      "A system can only request resources and queries!");
        return T();
    }
}

}
