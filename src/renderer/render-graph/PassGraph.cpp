#include "renderer/render-graph/PassGraph.h"

#include <algorithm>
#include <numeric>
#include <ranges>

Cel::Renderer::PassGraph::PassGraph(std::vector<RenderPass>& passes)
    : passes(passes)
{

    // Firstly, we create a map stating which resources belong to which passes
    // Additionally we create a mapping from the resource name to its id
    uint32_t resourceHandle = 0;

    auto init = [&](const auto& name, auto passId) {
        // map this resource to the pass that owns it
        resourceToPass[{ resourceHandle }] = passId;
        // map the resource name to its handle
        nameToResource[name] = { resourceHandle };

        resourceHandle++;
    };

    for (const auto& [i, pass] : std::views::enumerate(passes)) {
        const Handle<RenderPass> passId = { static_cast<uint32_t>(i) };
        internal.add_node(passId);

        for (const auto& create : pass.newBuffers) {
            init(create.name, passId);
        }
        for (const auto& create : pass.newImages) {
            init(create.name, passId);
        }

        // Mark how many of our nodes are root
        numOfRootNodes = resourceHandle;

        for (const auto& write : pass.bufferWrites) {
            init(write.outName, passId);
        }
        for (const auto& write : pass.imageWrites) {
            init(write.outName, passId);
        }
    }

    // After the basic initialisation we can actually construct the graph
    for (const auto& [i, pass] : std::views::enumerate(passes)) {
        const Handle<RenderPass> to = { static_cast<uint32_t>(i) };

        // Add the previous pass as a dependency to this pass on write
        for (const auto& write : pass.bufferWrites) {
            const auto from = get_owner_pass(write.inName);
            internal.add_edge(from, to, nameToResource[write.inName]);
        }
        for (const auto& write : pass.imageWrites) {
            const auto from = get_owner_pass(write.inName);
            internal.add_edge(from, to, nameToResource[write.inName]);
        }
    }

    // Add dependency from pass that reads x to pass that writes to x
    for (const auto& [i, pass] : std::views::enumerate(passes)) {
        const Handle<RenderPass> from = { static_cast<uint32_t>(i) };

        for (const auto& read : pass.bufferReads) {
            const auto id = nameToResource[read.name];

            const auto to = internal.get_destination_node(from, id);
            internal.add_edge(from, to, id);
        }
        for (const auto& read : pass.imageReads) {
            const auto id = nameToResource[read.name];

            const auto to = internal.get_destination_node(from, id);
            internal.add_edge(from, to, id);
        }
    }
}

Cel::Handle<Cel::Renderer::RenderPass>
Cel::Renderer::PassGraph::get_owner_pass(const std::string& name)
{
    return resourceToPass[nameToResource[name]];
}

Cel::Renderer::PassGraph::Iterator::Iterator(PassGraph& graph)
    : graph(graph)
{
    criticalPaths.resize(graph.passes.size());

    for (uint32_t i = 0; i <= graph.numOfRootNodes; i++) {
        calculate_critical_path_lengths({ i });
    }

    for (uint32_t i = 0; i <= graph.numOfRootNodes; i++) {
        readyNodes.emplace(criticalPaths[i], i);
        add_dependents({ i });
    }
}

Cel::Renderer::PassGraph::Iterator
Cel::Renderer::PassGraph::iter()
{
    return Iterator(*this);
}

void
Cel::Renderer::PassGraph::Iterator::mark_finished(Handle<RenderPass> pass)
{
    finished.insert(pass);

    add_dependents(pass);
}

void
Cel::Renderer::PassGraph::Iterator::add_dependents(Handle<RenderPass> pass)
{
    // queue up next nodes in graph
    const auto& dependents = graph.internal.get_dependents(pass);

    for (auto dependent : dependents) {
        const auto& requirements = graph.internal.get_dependencies(dependent);

        // if all requirements are met
        if (std::ranges::all_of(requirements, [&](auto handle) {
                return finished.contains(handle);
            })) {

            readyNodes.emplace(criticalPaths[dependent.index], dependent);
            // Check if we can add any child nodes as well
            add_dependents(dependent);
        }
    }
}

std::vector<Cel::Handle<Cel::Renderer::RenderPass>>&
Cel::Renderer::PassGraph::Iterator::get_best_nodes()
{
    bestNodes.clear();
    if (readyNodes.empty()) {
        return bestNodes;
    }

    bestNodes.clear();
    auto iter = readyNodes.rbegin();

    const auto& [longestPath, firstPass] = *iter;

    bestNodes.push_back(firstPass);

    ++iter;

    for (; iter != readyNodes.rend(); ++iter) {
        const auto& [length, pass] = *iter;

        if (longestPath == length) {
            bestNodes.push_back(pass);
            continue;
        }

        break;
    }

    return bestNodes;
}

std::set<std::pair<uint32_t, Cel::Handle<Cel::Renderer::RenderPass>>>&
Cel::Renderer::PassGraph::Iterator::get_available_nodes()
{
    return readyNodes;
}

Cel::Renderer::PassGraph::Iterator
Cel::Renderer::PassGraph::Iterator::branch_off()
{

    return { *this };
}

uint32_t
Cel::Renderer::PassGraph::Iterator::calculate_critical_path_lengths(
    const Handle<RenderPass> pass)
{
    // TODO : Add some logic that culls nodes that don't reach our end point
    // i.e. passes that have no effect on the final render
    if (criticalPaths[pass.index] != 0) {
        return criticalPaths[pass.index];
    }

    const auto& dependents = graph.internal.get_dependents(pass);

    uint32_t greatestPath = 0;
    for (const auto& dep : dependents) {
        greatestPath =
            std::max(calculate_critical_path_lengths(dep), greatestPath);
    }

    greatestPath++;

    criticalPaths[pass.index] = greatestPath;

    return greatestPath;
}

void
Cel::Renderer::PassGraph::Iterator::reset()
{
    bestNodes.clear();
    readyNodes.clear();
}
