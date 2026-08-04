#include "antwika/life/DragPausedSystem.hpp"

namespace antwika::life
{

    DragPausedSystem::DragPausedSystem(ISystem &inner, const DragState &drag)
        // Staging nothing is what holds the board still.
        // The commit after this phase then finds only what the drag did.
        : gate(inner, [&drag] { return !drag.inProgress(); })
    {
    }

    void DragPausedSystem::update(World &world, antwika::time::Tick tick)
    {
        gate.update(world, tick);
    }

} // namespace antwika::life
