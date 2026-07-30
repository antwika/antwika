#include "antwika/life/DragPausedSystem.hpp"

namespace antwika::life
{

    DragPausedSystem::DragPausedSystem(ISystem &inner, const DragState &drag)
        : inner(inner), drag(drag)
    {
    }

    void DragPausedSystem::update(World &world, antwika::time::Tick tick)
    {
        // Staging nothing is what holds the board still.
        // The commit after this phase then finds only what the drag did.
        if (drag.inProgress())
        {
            return;
        }

        inner.update(world, tick);
    }

} // namespace antwika::life
