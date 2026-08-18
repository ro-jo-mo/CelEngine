#pragma once

#include "Graph.h"

#include <map>
#include <vector>

namespace Cel::Common
{
    template <typename Key, typename Graph>
    class RelativeScheduler
    {
    public:
        // Single system
        RelativeScheduler(Graph& graph, Key system)
            : graph(graph)
              , exit{system}
              , entrance{system}
        {
        }

        // Chain
        RelativeScheduler(Graph& graph, Key start, Key end)
            : graph(graph)
              , exit{end}
              , entrance{start}
        {
        }

        // Group of systems
        RelativeScheduler(Graph& graph, std::vector<Key>&& group)
            : graph(graph)
              , exit(group)
              , entrance(group)
        {
        }

        RelativeScheduler& after(const RelativeScheduler& runsBefore);

        // The completely generic "node" parameter is specifically built for system
        // scheduling As we need the actual type info of the function pointer
        // (almost certainly unique per system) it is kept completely generic, and
        // handled by the graph
        template <typename Node>
        RelativeScheduler& after(Node runsBefore);

        RelativeScheduler& before(const RelativeScheduler& runsAfter);

        template <typename Node>
        RelativeScheduler& before(Node runsAfter);

    private:
        Graph& graph;
        std::vector<Key> exit;
        std::vector<Key> entrance;
    };

    template <typename Key, typename Graph>
    RelativeScheduler<Key, Graph>&
    RelativeScheduler<Key, Graph>::after(const RelativeScheduler& runsBefore)
    {
        // The simplest way of reasoning about this is to think of a chain
        // Our set:
        // a (entrance) -> b -> c -> d (exit)
        // Scheduling this set after some other, runsBefore's exit becomes a's
        // dependencies

        for (const auto& entry : this->entrance)
        {
            for (const auto& exits : runsBefore.exit)
            {
                graph.add_edge(exits, entry);
            }
        }

        return *this;
    }

    template <typename Key, typename Graph>
    template <typename Node>
    RelativeScheduler<Key, Graph>&
    RelativeScheduler<Key, Graph>::after(Node runsBefore)
    {
        graph.add_node(runsBefore);

        for (const auto& entry : this->entrance)
        {
            graph.add_edge(reinterpret_cast<Key>(runsBefore), entry);
        }

        return *this;
    }

    template <typename Key, typename Graph>
    RelativeScheduler<Key, Graph>&
    RelativeScheduler<Key, Graph>::before(const RelativeScheduler& runsAfter)
    {
        // The simplest way of reasoning about this is to think of a chain
        // Our set:
        // a (entrance) -> b -> c -> d (exit)
        // Scheduling this set before some other, d becomes the requirement for
        // runsAfters entrance

        for (const auto& exits : this->exit)
        {
            for (const auto& entry : runsAfter.entrance)
            {
                graph.add_edge(exits, entry);
            }
        }

        return *this;
    }

    template <typename Key, typename Graph>
    template <typename Node>
    RelativeScheduler<Key, Graph>&
    RelativeScheduler<Key, Graph>::before(Node runsAfter)
    {
        graph.add_node(runsAfter);

        for (const auto& exits : this->exit)
        {
            graph.add_edge(exits, reinterpret_cast<Key>(runsAfter));
        }

        return *this;
    }

    namespace Detail
    {
        // The base which schedulers use
        // Due to pains with system scheduling needing type information ...
        // It doesn't add the nodes to graph
        template <typename Key>
        class BaseScheduler
        {
        protected:
            template <typename Graph>
            static RelativeScheduler<Key, Graph> add_system_impl(Graph& graph, Key key);

            template <typename Graph, typename... Keys>
            static RelativeScheduler<Key, Graph> add_group_impl(Graph& graph,
                                                                Keys... keys);

            template <typename Graph, typename... Keys>
            static RelativeScheduler<Key, Graph> add_chain_impl(Graph& graph,
                                                                Keys... keys);
        };

        template <typename Key>
        template <typename Graph>
        RelativeScheduler<Key, Graph>
        BaseScheduler<Key>::add_system_impl(Graph& graph, Key key)
        {
            return {graph, key};
        }

        template <typename Key>
        template <typename Graph, typename... Keys>
        RelativeScheduler<Key, Graph>
        BaseScheduler<Key>::add_group_impl(Graph& graph, Keys... keys)
        {
            return {graph, {keys...}};
        }

        template <typename Key>
        template <typename Graph, typename... Keys>
        RelativeScheduler<Key, Graph>
        BaseScheduler<Key>::add_chain_impl(Graph& graph, Keys... keys)
        {
            auto tuple = std::make_tuple(keys...);
            constexpr size_t SIZE = sizeof...(keys);

            [&]<size_t... Index>(std::index_sequence<Index...>)
            {
                (void(graph.add_edge(std::get<Index>(tuple),
                                     std::get<Index + 1>(tuple))),
                    ...);
            }(std::make_index_sequence<SIZE - 1>{});

            auto first = std::get<0>(tuple);
            auto last = std::get<SIZE - 1>(tuple);

            return RelativeScheduler<Key, Graph>{graph, first, last};
        }
    }

    // A semi generic scheduler
    // Creates a directed acyclic graph representing the partial ordering
    // of passes / systems
    template <typename Key>
    class Scheduler : Detail::BaseScheduler<Key>
    {
    public:
        template <typename System>
        RelativeScheduler<Key, Scheduler> add_system(System system);

        template <typename... Systems>
        RelativeScheduler<Key, Scheduler> add_group(Systems... systems);

        template <typename... Systems>
        RelativeScheduler<Key, Scheduler> add_chain(Systems... systems);

    protected:
        Graph<Key> graph;
    };

    template <typename Key>
    template <typename System>
    RelativeScheduler<Key, Scheduler<Key>>
    Scheduler<Key>::add_system(System system)
    {
        graph.add_node(system);
        return Detail::BaseScheduler<Key>::add_system_impl(graph, system);
    }

    template <typename Key>
    template <typename... Systems>
    RelativeScheduler<Key, Scheduler<Key>>
    Scheduler<Key>::add_group(Systems... systems)
    {
        (void(graph.add_node(systems)), ...);
        return add_group_impl(graph, systems...);
    }

    template <typename Key>
    template <typename... Systems>
    RelativeScheduler<Key, Scheduler<Key>>
    Scheduler<Key>::add_chain(Systems... systems)
    {
        (void(graph.add_node(systems)), ...);
        return add_chain_impl(graph, systems...);
    }
}
