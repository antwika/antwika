#include "antwika/tower_defence/Battle.hpp"

#include <utility>

namespace antwika::tower_defence
{

    namespace
    {
        // The run's own table rather than the shipped constant.
        // A config states one set of mobs for the whole campaign.
        [[nodiscard]] const MobProfile &profileFor(
            const BattleConfig &config, const MobKind kind)
        {
            return config.mobs[static_cast<std::size_t>(kind)];
        }

        std::uint32_t squaredDistance(const Cell &left, const Cell &right)
        {
            const auto dx = static_cast<std::int64_t>(left.x)
                - static_cast<std::int64_t>(right.x);
            const auto dy = static_cast<std::int64_t>(left.y)
                - static_cast<std::int64_t>(right.y);
            return static_cast<std::uint32_t>((dx * dx) + (dy * dy));
        }

        // Armour comes off before the hit lands, and a hit never heals.
        std::int32_t damageTo(
            const std::int32_t towerDamage, const std::int32_t armour)
        {
            return towerDamage > armour ? towerDamage - armour : 0;
        }
    } // namespace

    Battle::Battle(
        Level level, BattleConfig config, std::vector<WaveRelease> waves)
        : levelData(std::move(level)),
          config(config),
          waves(std::move(waves))
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

    StepOutcome Battle::step()
    {
        const std::uint32_t leaks = walk();
        release();
        const std::uint64_t reward = fire();
        ++tickCount;
        return StepOutcome{.reward = reward, .leaks = leaks};
    }

    bool Battle::cleared() const
    {
        return waveIndex >= waves.size() && living.empty();
    }

    std::uint32_t Battle::walk()
    {
        std::uint32_t leaked = 0;
        std::vector<Mob> survivors;
        survivors.reserve(living.size());
        for (Mob &mob : living)
        {
            // A kind's pace is how many ticks one cell costs it.
            // A Runner at one crosses every tick, as every mob used to.
            // A Brute at three stands still for two of every three.
            if (mob.ticksUntilStep > 0)
            {
                --mob.ticksUntilStep;
                survivors.push_back(mob);
                continue;
            }

            ++mob.pathIndex;
            mob.ticksUntilStep =
                profileFor(config, mob.kind).ticksPerCell - 1;
            if (mob.pathIndex + 1 < levelData.path.size())
            {
                survivors.push_back(mob);
                continue;
            }

            // Reaching the end is the one way a mob costs the player.
            // What it costs is a life, and Campaign is what keeps those.
            ++leaked;
        }
        living = std::move(survivors);
        return leaked;
    }

    void Battle::release()
    {
        if (waveIndex >= waves.size())
        {
            return;
        }
        if (ticksUntilRelease > 0)
        {
            --ticksUntilRelease;
            return;
        }

        const WaveRelease &wave = waves[waveIndex];

        // A level with no path gives a mob no cell to stand on.
        // Both fire() and snapshotOf() read the cell one stands on.
        // Releasing nothing keeps either from being reachable.
        // Letting the wave pass anyway is what stops such a level.
        // Otherwise a campaign could never clear it.
        if (spawnedInWave < wave.order.size()
            && !levelData.path.empty())
        {
            const MobKind kind = wave.order[spawnedInWave];
            Mob released;
            released.id = nextMobId;
            released.kind = kind;
            released.health = profileFor(config, kind).health;
            living.push_back(released);
            ++nextMobId;
            ++spawnedInWave;

            if (spawnedInWave < wave.order.size())
            {
                ticksUntilRelease = wave.spawnPeriodTicks;
                return;
            }
        }

        // The wave is spent, so the gap before the next one starts.
        ++waveIndex;
        spawnedInWave = 0;
        ticksUntilRelease = wave.gapTicks;
    }

    std::uint64_t Battle::fire()
    {
        for (const Tower &tower : guns)
        {
            // Furthest along the path wins.
            // That is the mob about to leak, so it is the one to shoot.
            //
            // The tie-break is written out rather than relied upon.
            // Mobs used to advance one cell a tick each.
            // So no two could ever share a path index.
            // And the first in reach was always the furthest along.
            // Kinds walk at different paces now.
            // So two mobs can stand on one cell.
            // And a Runner can overtake a Brute.
            // With no tie-break, vector order alone would decide.
            // living is kept in ascending spawn order.
            // Only a strictly greater path index replaces a candidate.
            // So a tie resolves to the smallest id.
            // Both of those are total orders.
            std::size_t best = living.size();
            for (std::size_t i = 0; i < living.size(); ++i)
            {
                const Cell &at = levelData.path[living[i].pathIndex];
                if (squaredDistance(at, tower.cell)
                    > config.towerRangeSquared)
                {
                    continue;
                }
                if (best == living.size()
                    || living[i].pathIndex > living[best].pathIndex)
                {
                    best = i;
                }
            }
            if (best == living.size())
            {
                continue;
            }

            living[best].health -= damageTo(
                config.towerDamage,
                profileFor(config, living[best].kind).armour);
        }

        std::uint64_t earned = 0;
        std::vector<Mob> survivors;
        survivors.reserve(living.size());
        for (const Mob &mob : living)
        {
            if (mob.health > 0)
            {
                survivors.push_back(mob);
                continue;
            }
            earned += profileFor(config, mob.kind).reward;
        }
        living = std::move(survivors);
        return earned;
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

    std::uint64_t Battle::ticks() const
    {
        return tickCount;
    }

    std::size_t Battle::waveCount() const
    {
        return waves.size();
    }

    std::size_t Battle::wavesReleased() const
    {
        return waveIndex;
    }

} // namespace antwika::tower_defence
