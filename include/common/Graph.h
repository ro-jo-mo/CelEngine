#pragma once
#include "Handle.h"
#include "core/Error.h"

#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>

namespace Cel::Common {

/**
 * @brief A semi generic graph implementation
 * This is primarily use for scheduling, as a DAG
 * Each node is represented by the key
 * Additionally there's the assumption that our edges are fairly sparse
 * As such it is only tested for this use case
 */
template<typename... Ts>
class Graph;

template<typename Key>
class Graph<Key>
{
  public:
    /**
     * @brief Add a new node to the graph, with no edges, with the given value
     * If the node already exists, does nothing
     */
    void add_node(Key key);

    /**
     * @brief  Add an edge marking "from" as a dependency for "to"
     * Does not require either node to exist presently
     * @param from
     * @param to
     */
    void add_edge(Key from, Key to);

    class Iterator;

    Iterator iter();

    std::unordered_set<Key>& get_dependencies(Key key);
    std::unordered_set<Key>& get_dependents(Key key);

    std::unordered_map<Key, std::unordered_set<Key>> adjacencyList{};
    std::unordered_map<Key, std::unordered_set<Key>> reverseAdjacencyList{};
};

template<typename Key>
void
Graph<Key>::add_node(Key key)
{
    // Initialise as empty, if exists do nothing
    reverseAdjacencyList[key];
    adjacencyList[key];
}

template<typename Key>
void
Graph<Key>::add_edge(Key from, Key to)
{
    adjacencyList[from].insert(to);
    reverseAdjacencyList[to].insert(from);
}

template<typename Key>
std::unordered_set<Key>&
Graph<Key>::get_dependencies(Key key)
{
    return reverseAdjacencyList[key];
}

template<typename Key>
std::unordered_set<Key>&
Graph<Key>::get_dependents(Key key)
{
    return adjacencyList[key];
}

/**
 * A lazily made graph variant that includes weights on each edge
 * Specialised for handles
 * @tparam Key
 * @tparam Weight
 */
template<typename KeyT, typename Weight>
class Graph<Handle<KeyT>, Weight> : public Graph<Handle<KeyT>>
{
    using Key = Handle<KeyT>;

  public:
    void add_edge(Key from, Key to, Weight weight);

    Weight get_edge(Key from, Key to);

    /**
     * @brief Get the node that has an edge from "from" to itself, with a weight
     * of weight
     * Likely very fragile, assumes no two edges from the same node have the
     * same weight
     * @param from
     * @param weight
     * @return
     */
    Key get_destination_node(Key from, Weight weight);

    std::unordered_map<uint64_t, Weight> weights;

  private:
    uint64_t make_edge_key(Key from, Key to);
};

template<typename KeyT, typename Weight>
void
Graph<Handle<KeyT>, Weight>::add_edge(Key from, Key to, Weight weight)
{
    Graph<Key>::add_edge(from, to);
    weights[make_edge_key(from, to)] = weight;
}

template<typename KeyT, typename Weight>
Weight
Graph<Handle<KeyT>, Weight>::get_edge(Key from, Key to)
{
    return weights[make_edge_key(from, to)];
}

template<typename KeyT, typename Weight>
Graph<Handle<KeyT>, Weight>::Key
Graph<Handle<KeyT>, Weight>::get_destination_node(Key from, Weight weight)
{
    // Interesting syntax :/
    for (const auto& to : Graph<Key>::adjacencyList[from]) {
        if (get_edge(from, to) == weight) {
            return to;
        }
    }

    throw_error("Could not find destination node in graph.");

    return {};
}

template<typename KeyT, typename Weight>
uint64_t
Graph<Handle<KeyT>, Weight>::make_edge_key(Key from, Key to)
{
    return (static_cast<uint64_t>(from.index) << 32) | to.index;
}

template<typename Key>
class Graph<Key>::Iterator
{
  public:
    explicit Iterator(Graph& graph)
        : graph(graph)
    {
        for (const auto& [key, reqs] : graph.reverseAdjacencyList) {
            if (reqs.empty()) {
                calculate_critical_path_lengths(key);
            }
        }
    }

    void mark_finished(Key key);

    void calculate_unused(Key endpoint);

    Iterator branch_off();

  private:
    uint32_t calculate_critical_path_lengths(Key root);

    void add_dependents(Key key);

    std::unordered_map<Key, uint32_t> criticalPaths;

    // We automatically cull nodes that are unused i.e. do not reach our
    // endpoint This is optional, and activated through "calculate_unused"
    std::unordered_set<Key> unused;

    // Nodes we've already executed
    std::unordered_set<Key> finished;

    // A pairing of the critical path length and key, used such that rbegin()
    // can iter the most important nodes first
    std::set<std::pair<uint32_t, Key>> readyNodes;

    Graph& graph;
};

template<typename Key>
Graph<Key>::Iterator
Graph<Key>::iter()
{
    return Iterator(*this);
}

template<typename Key>
void
Graph<Key>::Iterator::mark_finished(Key key)
{
    // Remove from ready nodes
    readyNodes.erase({ criticalPaths[key], key });
    // Add dependents
    add_dependents(key);
}

template<typename Key>
void
Graph<Key>::Iterator::calculate_unused(Key endpoint)
{
    std::unordered_set<Key> used;

    auto recurse = [&](const Key key) {
        used.insert(key);

        auto& dependencies = graph.get_dependencies(key);
        for (const auto& dep : dependencies) {
            recurse(dep);
        }
    };

    recurse(endpoint);

    // I'd assume it is slightly cheaper to check a node is !unusued that is
    // used On the assumption unused will be far smaller than used

    for (const auto& key : graph.adjacencyList | std::views::keys) {
        if (!used.contains(key)) {
            unused.insert(key);
        }
    }
}

template<typename Key>
Graph<Key>::Iterator
Graph<Key>::Iterator::branch_off()
{
    return Iterator(this);
}

template<typename Key>
uint32_t
Graph<Key>::Iterator::calculate_critical_path_lengths(Key root)
{
    // i.e. passes that have no effect on the final render
    if (criticalPaths[root] != 0) {
        return criticalPaths[root];
    }

    const auto& dependents = graph.get_dependents(root);

    uint32_t greatestPath = 0;
    for (const auto& dep : dependents) {
        greatestPath =
            std::max(calculate_critical_path_lengths(dep) + 1, greatestPath);
    }

    criticalPaths[root] = greatestPath;

    return greatestPath;
}

template<typename Key>
void
Graph<Key>::Iterator::add_dependents(Key key)
{
    const auto& dependents = graph.get_dependents(key);

    for (const auto& dep : dependents) {
        const auto& requirements = graph.get_dependencies(dep);

        if (const auto& requirements = graph.get_dependencies(dep);
            std::all_of(requirements.begin(), requirements.end(), [&](auto x) {
                return finished.count(x);
            })) {
            readyNodes.emplace({ criticalPaths[dep], key });
        }
    }
}

};
