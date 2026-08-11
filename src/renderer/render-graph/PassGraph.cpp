#include "renderer/render-graph/PassGraph.h"

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
            internal.add_edge(from, to, nameToResource[write.outName]);
        }
        for (const auto& write : pass.imageWrites) {
            const auto from = get_owner_pass(write.inName);
            internal.add_edge(from, to, nameToResource[write.outName]);
        }
    }

    auto writeSequences = internal.adjacencyList;

    for (const auto& [i, pass] : std::views::enumerate(passes)) {
        const Handle<RenderPass> from = { static_cast<uint32_t>(i) };

        // Add this pass as a dependency when another pass wishes to write to a
        // resource read by it
        for (const auto& read : pass.bufferReads) {
            // Get first (and only value if graph is correct)
            auto& edges = writeSequences[get_owner_pass(read.name)];

            // Check there exists another pass that wants to write this resource
            // If so we add an edge
            if (edges.size() == 1) {
                const auto to = *edges.begin();
                internal.add_edge(from, to, { 1 });
            } else {
                // The only valid cases are that this is the last node in a
                // sequence  or has exactly 1 edge
                assert(edges.empty());
            }
        }
    }
}

Cel::Handle<Cel::Renderer::RenderPass>
Cel::Renderer::PassGraph::get_owner_pass(const std::string& name)
{
    return resourceToPass[nameToResource[name]];
}

uint32_t
Cel::Renderer::PassGraph::Iterator::calculate_critical_path_lengths(
    const Handle<RenderPass> pass)
{
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
