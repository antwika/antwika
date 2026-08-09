#include "antwika/life/DragPausedSystem.hpp"

namespace antwika::life
{

    DragPausedSystem::DragPausedSystem(ISystem &inner, const DragState &drag)
        : gate(inner, [&drag] { return !drag.inProgress(); })
    {
    }

    void DragPausedSystem::update(World &world, antwika::time::Tick tick)
    {
        gate.update(world, tick);
    }

}
