#include "ecs/Scheduler.h"

Cel::RelativeScheduler&
Cel::RelativeScheduler::after(const RelativeScheduler& runsBefore)
{
    // The simplest way of reasoning about this is to think of a chain
    // Our set:
    // a (entrance) -> b -> c -> d (exit)
    // Scheduling this set after some other, runsBefore's exit becomes a's
    // dependencies

    for (const auto& entry : this->entrance) {
        for (const auto& exits : runsBefore.exit) {
            graph.add_edge(exits, entry);
        }
    }

    return *this;
}

Cel::RelativeScheduler&
Cel::RelativeScheduler::before(const RelativeScheduler& runsAfter)
{
    // The simplest way of reasoning about this is to think of a chain
    // Our set:
    // a (entrance) -> b -> c -> d (exit)
    // Scheduling this set before some other, d becomes the requirement for
    // runsAfters entrance

    for (const auto& exits : this->exit) {
        for (const auto& entry : runsAfter.entrance) {
            graph.add_edge(exits, entry);
        }
    }

    return *this;
}
