#include "ecs/Scheduler.h"

Cel::RelativeScheduler&
Cel::RelativeScheduler::After(const RelativeScheduler& runsBefore)
{
    // The simplest way of reasoning about this is to think of a chain
    // Our set:
    // a (entrance) -> b -> c -> d (exit)
    // Scheduling this set after some other, runsBefore's exit becomes a's
    // dependencies

    for (const auto& entry : this->entrance) {
        for (const auto& exits : runsBefore.exit) {
            graph.AddEdge(exits, entry);
        }
    }

    return *this;
}

Cel::RelativeScheduler&
Cel::RelativeScheduler::After(const void* runsBefore)
{
    for (const auto& entry : this->entrance) {
        graph.AddEdge(runsBefore, entry);
    }

    return *this;
}

Cel::RelativeScheduler&
Cel::RelativeScheduler::Before(const RelativeScheduler& runsAfter)
{
    // The simplest way of reasoning about this is to think of a chain
    // Our set:
    // a (entrance) -> b -> c -> d (exit)
    // Scheduling this set before some other, d becomes the requirement for
    // runsAfters entrance

    for (const auto& exits : this->exit) {
        for (const auto& entry : runsAfter.entrance) {
            graph.AddEdge(exits, entry);
        }
    }

    return *this;
}

Cel::RelativeScheduler&
Cel::RelativeScheduler::Before(const void* runsAfter)
{
    for (const auto& exits : this->exit) {
        graph.AddEdge(exits, runsAfter);
    }

    return *this;
}