#pragma once
#include "Handle.h"
#include "core/Error.h"

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

};
