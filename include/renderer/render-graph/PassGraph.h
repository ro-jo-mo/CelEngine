#pragma once

#include "RenderPass.h"
#include "common/Graph.h"
#include "renderer/VulkanTypes.h"

#include <set>

namespace Cel::Renderer {
// Graph of render passes:
// Each render pass has edges based on its writes
// We need some form of indirection to writes, such that a pass can take
// ownership of writes (a,b,c,...)
// Requires some id for writes, we should be able to use the out name handle,
// and use a flag bit to detect if image / buffer
// Does read ordering need consideration?
// Suppose pass A writes to "www", the only dependency for pass B & C
// B must read "xxx", which has not yet been created, so C runs
// Pass D has pass C as its dependency, and writes to "xxx" outputting "yyy"
// For a pass that transforms "xxx" to "yyy", all passes that read "xxx" are
// a dependency

// Overall, it would seem easiest if the graph was constructed at all once, from
// a list of render passes
// To start, for each pass assign a unique id, and a map of writes -> pass

// This simplifies the rest of the process, in which we can create edges between
// nodes as expected

// This graph represents purely the ordering of passes
class PassGraph
{
  public:
    explicit PassGraph(std::vector<RenderPass>& passes);

    struct Iterator;

    Iterator iter();

  private:
    Handle<RenderPass> get_owner_pass(const std::string& name);

    // A mapping of resource names to a handle
    // Dummy struct for type safety
    // Focusing entirely on pass ordering, we don't need to distinguish between
    // buffer and image resources
    // As such they're both grouped under a unified resource handle
    struct Resource
    {};
    std::unordered_map<std::string, Handle<Resource>> nameToResource;

    // A mapping of resources to the pass that creates it
    std::unordered_map<Handle<Resource>, Handle<RenderPass>> resourceToPass;

    // The internal graph structure
    // Nodes are passes and edge weights identify the
    // resource dependency (read or written to)
    Common::Graph<Handle<RenderPass>, Handle<Resource>> internal;

    // As we allocate handles in an ordered way, we can check whether a node is
    // a root via comparison
    // isRoot = handle.id <= numOfRootNodes
    uint32_t numOfRootNodes;

    // A list of render passes
    // Indexed using handles ...
    std::vector<RenderPass>& passes;
};

// Separate class for deciding pass order
// Simply returns available nodes, in the form of a priority queue
// Them we generate an execution plan
// We need to track the state of each resource, what is the current image
// layout, queue family, access flags(?)
struct PassGraph::Iterator
{
    using PassGroup = std::vector<std::reference_wrapper<Handle<RenderPass>>>;

    explicit Iterator(PassGraph& graph);

    // Mark this pass as complete
    void mark_finished(Handle<RenderPass> pass);

    std::vector<Handle<RenderPass>>& get_best_nodes();

    std::set<std::pair<uint32_t, Handle<RenderPass>>>& get_available_nodes();

    // Create a copy of this iterator, to search another branch
    Iterator branch_off();

    void reset();

  private:
    uint32_t calculate_critical_path_lengths(Handle<RenderPass> pass);

    void add_dependents(Handle<RenderPass> pass);

    PassGraph& graph;

    // Of equal size to graph.passes
    // The longest path from this pass to an end node
    std::vector<uint32_t> criticalPaths;

    // An ordered set of render passes, based on their critical path length
    std::set<std::pair<uint32_t, Handle<RenderPass>>> readyNodes;
    // Stored just so we can reuse the same memory
    std::vector<Handle<RenderPass>> bestNodes;

    std::unordered_set<Handle<RenderPass>> finished;
};

}