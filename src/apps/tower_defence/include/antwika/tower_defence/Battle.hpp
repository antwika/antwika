#pragma once

#include <array>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/Wave.hpp"

namespace antwika::tower_defence
{

    struct Mob final
    {
        std::uint32_t id = 0;

        MobKind kind = MobKind::Grunt;

        std::size_t pathIndex = 0;

        std::int32_t health = 0;

        std::uint32_t ticksUntilStep = 0;

        [[nodiscard]] bool operator==(const Mob &) const = default;
    };

    struct Tower final
    {
        std::uint32_t id = 0;
        Cell cell;

        [[nodiscard]] bool operator==(const Tower &) const = default;
    };

    struct BattleMemory final
    {
        std::size_t waveIndex = 0;

        std::size_t spawnedInWave = 0;

        std::uint64_t ticksUntilRelease = 0;

        std::uint64_t tickCount = 0;

        std::uint32_t nextMobId = 0;

        std::uint32_t nextTowerId = 0;

        std::vector<Mob> mobs;

        std::vector<Tower> towers;

        [[nodiscard]] bool operator==(const BattleMemory &) const
            = default;
    };

    struct BattleConfig final
    {
        std::uint32_t towerRangeSquared = 4;

        std::array<MobProfile, kMobKindCount> mobs = kDefaultMobProfiles;

        std::int32_t towerDamage = 1;
    };

    struct StepOutcome final
    {
        std::uint64_t reward = 0;

        std::uint32_t leaks = 0;
    };

    class Battle final
    {
    public:
        Battle(
            Level level,
            BattleConfig config,
            std::vector<WaveRelease> waves);

        bool placeTower(const Cell &cell);

        StepOutcome step();

        [[nodiscard]] bool cleared() const;

        [[nodiscard]] const Level &level() const;
        [[nodiscard]] const BattleConfig &settings() const;
        [[nodiscard]] const std::vector<Mob> &mobs() const;
        [[nodiscard]] const std::vector<Tower> &towers() const;
        [[nodiscard]] std::uint64_t ticks() const;

        [[nodiscard]] std::size_t waveCount() const;

        [[nodiscard]] std::size_t wavesReleased() const;

        [[nodiscard]] BattleMemory remember() const;

        [[nodiscard]] bool restore(const BattleMemory &memory);

    private:
        Level levelData;
        BattleConfig config;
        std::vector<WaveRelease> waves;
        std::vector<Mob> living;
        std::vector<Tower> guns;
        std::uint64_t tickCount = 0;
        std::uint64_t ticksUntilRelease = 0;
        std::size_t waveIndex = 0;
        std::size_t spawnedInWave = 0;
        std::uint32_t nextMobId = 0;
        std::uint32_t nextTowerId = 0;

        std::uint32_t walk();
        void release();
        std::uint64_t fire();
    };

}
