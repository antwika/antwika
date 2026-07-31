#include "antwika/tower_defence/Battle.hpp"

#include <algorithm>
#include <utility>

namespace antwika::tower_defence
{

    namespace
    {
        std::uint32_t squaredDistance(const Cell &left, const Cell &right)
        {
            const auto dx = static_cast<std::int64_t>(left.x)
                - static_cast<std::int64_t>(right.x);
            const auto dy = static_cast<std::int64_t>(left.y)
                - static_cast<std::int64_t>(right.y);
            return static_cast<std::uint32_t>((dx * dx) + (dy * dy));
        }
    } // namespace

    Battle::Battle(Level level, BattleConfig config)
        : levelData(std::move(level)), config(config)
    {
    }

    bool Battle::placeTower(const Cell &cell)
    {
        if (cell.x >= levelData.width || cell.y >= levelData.height)
        {
            return false;
        }
        if (levelData.at(cell) != Tile::Empty)
        {
            return false;
        }
        for (const Tower &tower : guns)
        {
            if (tower.cell == cell)
            {
                return false;
            }
        }
        guns.push_back(Tower{.id = nextTowerId, .cell = cell});
        ++nextTowerId;
        return true;
    }

    void Battle::step()
    {
        walk();
        spawn();
        fire();
        ++tickCount;
    }

    void Battle::walk()
    {
        std::vector<Mob> survivors;
        survivors.reserve(living.size());
        for (Mob &mob : living)
        {
            ++mob.pathIndex;
            if (mob.pathIndex + 1 < levelData.path.size())
            {
                survivors.push_back(mob);
                continue;
            }

            // Reaching the end is the one way a mob costs the player.
            // The score floors at zero rather than wrapping.
            ++leaked;
            totalScore -= std::min(totalScore, config.leakPenalty);
        }
        living = std::move(survivors);
    }

    void Battle::spawn()
    {
        if (tickCount % config.spawnPeriodTicks != 0)
        {
            return;
        }
        living.push_back(Mob{
            .id = nextMobId,
            .pathIndex = 0,
            .health = config.mobHealth});
        ++nextMobId;
    }

    void Battle::fire()
    {
        for (const Tower &tower : guns)
        {
            // Furthest along the path wins.
            // That is the mob about to leak, so it is the one to shoot.
            // No tie-break is needed, and none is written.
            // Mobs are kept in ascending spawn order.
            // Every mob advances exactly one cell per tick.
            // So two mobs can never share a path index.
            // The order is total by construction, not by a rule.
            std::size_t best = living.size();
            for (std::size_t i = 0; i < living.size(); ++i)
            {
                const Cell &at = levelData.path[living[i].pathIndex];
                if (squaredDistance(at, tower.cell)
                    > config.towerRangeSquared)
                {
                    continue;
                }
                const bool better = best == living.size()
                    || living[i].pathIndex > living[best].pathIndex;
                if (better)
                {
                    best = i;
                }
            }
            if (best == living.size())
            {
                continue;
            }
            living[best].health -= config.towerDamage;
        }

        std::vector<Mob> survivors;
        survivors.reserve(living.size());
        for (const Mob &mob : living)
        {
            if (mob.health > 0)
            {
                survivors.push_back(mob);
                continue;
            }
            totalScore += config.killScore;
        }
        living = std::move(survivors);
    }

    const Level &Battle::level() const
    {
        return levelData;
    }

    const BattleConfig &Battle::settings() const
    {
        return config;
    }

    const std::vector<Mob> &Battle::mobs() const
    {
        return living;
    }

    const std::vector<Tower> &Battle::towers() const
    {
        return guns;
    }

    std::uint64_t Battle::score() const
    {
        return totalScore;
    }

    std::uint64_t Battle::ticks() const
    {
        return tickCount;
    }

    std::uint32_t Battle::leaks() const
    {
        return leaked;
    }

} // namespace antwika::tower_defence
