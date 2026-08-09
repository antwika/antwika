#include "antwika/tower_defence/Battle.hpp"

#include <utility>

namespace antwika::tower_defence
{

    namespace
    {
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

        std::int32_t damageTo(
            const std::int32_t towerDamage, const std::int32_t armour)
        {
            return towerDamage > armour ? towerDamage - armour : 0;
        }
    }

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

        ++waveIndex;
        spawnedInWave = 0;
        ticksUntilRelease = wave.gapTicks;
    }

    std::uint64_t Battle::fire()
    {
        for (const Tower &tower : guns)
        {
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

    BattleMemory Battle::remember() const
    {
        return BattleMemory{ // GCOVR_EXCL_LINE
            .waveIndex = waveIndex,
            .spawnedInWave = spawnedInWave,
            .ticksUntilRelease = ticksUntilRelease,
            .tickCount = tickCount,
            .nextMobId = nextMobId,
            .nextTowerId = nextTowerId,
            .mobs = living,
            .towers = guns};
    } // GCOVR_EXCL_LINE

    bool Battle::restore(const BattleMemory &memory)
    {
        for (const Mob &mob : memory.mobs)
        {
            if (mob.pathIndex >= levelData.path.size())
            {
                return false;
            }
        }

        waveIndex = memory.waveIndex;
        spawnedInWave = memory.spawnedInWave;
        ticksUntilRelease = memory.ticksUntilRelease;
        tickCount = memory.tickCount;
        nextMobId = memory.nextMobId;
        nextTowerId = memory.nextTowerId;
        living = memory.mobs;
        guns = memory.towers;
        return true;
    }

}
