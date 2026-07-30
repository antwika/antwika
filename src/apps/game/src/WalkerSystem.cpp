#include "antwika/game/WalkerSystem.hpp"

#include "antwika/game/Cell.hpp"
#include "antwika/game/Walker.hpp"
#include "antwika/game/Walking.hpp"

namespace antwika::game
{

    WalkerSystem::WalkerSystem(const PathIndex &paths) : paths(paths)
    {
    }

    void WalkerSystem::update(World &world, antwika::time::Tick)
    {
        for (const auto entity : world.view<Walker, Cell>())
        {
            const auto at = world.get<Cell>(entity);
            const auto facing = world.get<Walker>(entity).facing;

            const auto heading =
                nextFacing(facing, paths.neighboursOf(at));

            if (!heading.has_value())
            {
                continue;
            }

            world.set<Walker>(entity, Walker{.facing = *heading});
            world.set<Cell>(entity, step(at, *heading));
        }
    }

} // namespace antwika::game
