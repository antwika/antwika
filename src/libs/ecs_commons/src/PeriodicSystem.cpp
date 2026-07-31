#include "antwika/ecs_commons/PeriodicSystem.hpp"

#include "antwika/ecs_commons/EcsCommonsError.hpp"

namespace antwika::ecs_commons
{

    PeriodicSystem::PeriodicSystem(
        ISystem &inner, antwika::time::Tick period, antwika::time::Tick offset)
        : inner(inner), period(period), offset(offset)
    {
        if (period == 0)
        {
            throw EcsCommonsError("PeriodicSystem: period must not be zero");
        }
    }

    bool PeriodicSystem::due(antwika::time::Tick tick) const noexcept
    {
        return tick % period == offset % period;
    }

    void PeriodicSystem::update(World &world, antwika::time::Tick tick)
    {
        if (!due(tick))
        {
            return;
        }

        inner.update(world, tick);
    }

} // namespace antwika::ecs_commons
