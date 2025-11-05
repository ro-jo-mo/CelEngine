#pragma once

#include <vector>
#include "Schedule.h"
#include "ScheduleGraph.h"

namespace Cel {
    class Scheduler {
    public:
        explicit Scheduler(auto &scdl) : schedules(scdl) {
        }

        template<typename First, typename Last>
        class SystemScheduler;

        template<typename System>
        SystemScheduler<System, System> AddSystem(Schedule schedule);

        template<typename First, typename Second, typename... Others>
        auto Chain(Schedule schedule);

    private:
        template<typename First, typename Second, typename... Others>

        void AddEdges(ScheduleGraph &graph);

        std::vector<ScheduleGraph> &schedules;
    };

    template<typename First, typename Last>
    class Scheduler::SystemScheduler {
    public:
        explicit SystemScheduler
        (ScheduleGraph &scdl) : schedule(scdl) {
        }

        template<typename T>
        SystemScheduler After();

        template<typename T>
        SystemScheduler Before();

    private:
        ScheduleGraph &schedule;
    };

    template<typename System>
    Scheduler::SystemScheduler<System, System> Scheduler::AddSystem(const Schedule schedule) {
        auto &graph = schedules[schedule];
        graph.AddNode<System>();
        return SystemScheduler<System, System>(graph);
    }

    template<typename First, typename Second, typename... Others>
    auto Scheduler::Chain(
        const Schedule schedule) {
        auto &graph = schedules[schedule];
        graph.AddNode<First>();
        graph.AddNode<Second>();
        (void(graph.AddNode<Others>()), ...);
        graph.AddEdge<First, Second>();
        if constexpr (sizeof...(Others) > 0) {
            AddEdges<Second, Others...>(graph);
            using Last = std::tuple_element_t<(sizeof...(Others) - 1), std::tuple<Others...> >;
            return SystemScheduler<First, Last>(graph);
        }
        return SystemScheduler<First, Second>(graph);
    }

    template<typename First, typename Second, typename... Others>
    void Scheduler::AddEdges(ScheduleGraph &graph) {
        graph.AddEdge<First, Second>();
        if (sizeof...(Others) > 0) {
            AddEdges<Second, Others...>(graph);
        }
    }


    template<typename First, typename Last>
    template<typename T>
    Scheduler::SystemScheduler<First, Last> Scheduler::SystemScheduler<First, Last>::After() {
        schedule.AddEdge<T, First>();
        return *this;
    }

    template<typename First, typename Last>
    template<typename T>
    Scheduler::SystemScheduler<First, Last> Scheduler::SystemScheduler<First, Last>::Before() {
        schedule.AddEdge<Last, T>();
        return *this;
    }
}
