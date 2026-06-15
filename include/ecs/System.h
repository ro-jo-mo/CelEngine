#pragma once

#include <functional>

#include "SystemAllocator.h"

namespace Cel {

namespace Detail {

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
FunctionType<ReturnT, ParameterTs...> GetFunctionType(
    ReturnT (*)(ParameterTs...));

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
// }

/**
 * @brief Abstract base class for game systems
 * @tparam Parameters A list of queries and resources required by this system
 */
template<typename... Parameters>
class System
{
  public:
    virtual ~System() = default;

    /**
     * @brief Registers this system, creating a new function with the required
     * data captured
     * @param system A system pointer, required for polymorphism
     * @param allocator Resource allocator for system
     * @return A function that runs this system with its required params.
     * Requires no inputs.
     */
    static std::function<void()> Register(std::shared_ptr<System> system,
                                          SystemAllocator& allocator);

    /**
     * @brief Virtual method for running the system.
     * @param params Queries, resources needed to run
     */
    virtual void Run(Parameters&... params) = 0;
};

template<typename... Parameters>
std::function<void()>
System<Parameters...>::Register(std::shared_ptr<System> system,
                                SystemAllocator& allocator)
{
    auto args = std::tuple<Parameters&...>(allocator.Register<Parameters>()...);
    auto run = [system](auto&... _args) { system->Run(_args...); };

    return [run, args = std::move(args)]() { std::apply(run, args); };
}

// Instead of creating a system class, we will simply pass a function pointer to
// add system

}
