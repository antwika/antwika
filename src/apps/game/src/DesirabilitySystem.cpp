#include "antwika/game/DesirabilitySystem.hpp"

namespace antwika::game
{

    DesirabilitySystem::DesirabilitySystem(
        DesirabilityField &field, GridExtent extent) noexcept
        : field(field), extent(extent)
    {
    }

    void DesirabilitySystem::update(World &world, antwika::time::Tick)
    {
        field = desirabilityFieldOf(world, extent);
    }

}
