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

    /** @brief One walker, crossing a path cell every few ticks. */
    struct Mob
    {
        std::uint32_t id = 0;

        MobKind kind = MobKind::Grunt;

        /** @brief How far along Level::path it has walked. */
        std::size_t pathIndex = 0;

        std::int32_t health = 0;

        /**
         * @brief Ticks left before it crosses into the next cell.
         *
         * Counted down rather than up so a mob spawns ready to move,
         * whatever its kind's pace is.
         */
        std::uint32_t ticksUntilStep = 0;

        [[nodiscard]] bool operator==(const Mob &) const = default;
    };

    /** @brief One gun, sitting on a cell no mob will ever walk. */
    struct Tower
    {
        std::uint32_t id = 0;
        Cell cell;

        [[nodiscard]] bool operator==(const Tower &) const = default;
    };

    /**
     * @brief Everything one battle has moved, and nothing it can
     * rebuild.
     *
     * The level and the wave plan are deliberately absent: both are
     * pure functions of the campaign's seed and the level index, so a
     * restore regenerates them exactly the way Campaign::buildBattle()
     * generated them and carries only the fight itself.
     */
    struct BattleMemory
    {
        /** @brief Which wave the release schedule stands in. */
        std::size_t waveIndex = 0;

        /** @brief How much of that wave is already out. */
        std::size_t spawnedInWave = 0;

        /** @brief Ticks left before the next release. */
        std::uint64_t ticksUntilRelease = 0;

        /** @brief How many ticks the battle has stepped. */
        std::uint64_t tickCount = 0;

        /** @brief The id the next released mob takes. */
        std::uint32_t nextMobId = 0;

        /** @brief The id the next placed tower takes. */
        std::uint32_t nextTowerId = 0;

        /** @brief Every mob still walking, in spawn order. */
        std::vector<Mob> mobs;

        /** @brief Every tower placed, in placement order. */
        std::vector<Tower> towers;

        [[nodiscard]] bool operator==(const BattleMemory &) const
            = default;
    };

    /** @brief The numbers one level's guns are balanced with. */
    struct BattleConfig
    {
        /**
         * @brief Squared reach of a tower, in cells.
         *
         * Squared so the check is integer throughout: no sqrt, and so no
         * floating point anywhere in the simulation.
         * A level sets its own, which is one of the things that makes
         * one level play differently from another: a short reach makes
         * where a tower goes the whole decision, and a long one makes
         * how many of them there are.
         */
        std::uint32_t towerRangeSquared = 4;

        /**
         * @brief What each mob kind is worth, costs and survives.
         *
         * Carried on the battle rather than read off a constant, so a
         * config file can restate it -- kDefaultMobProfiles is what a
         * run that says nothing plays.
         */
        std::array<MobProfile, kMobKindCount> mobs = kDefaultMobProfiles;

        /**
         * @brief Health a tower takes off its target each tick.
         *
         * A mob's armour comes off this before it lands, so a level
         * whose guns do not out-damage an armoured kind cannot kill one
         * at all -- see MobProfile::armour.
         */
        std::int32_t towerDamage = 1;
    };

    /** @brief What one step of a battle came to. */
    struct StepOutcome
    {
        /** @brief Score earned by kills this tick. */
        std::uint64_t reward = 0;

        /** @brief How many mobs reached the end this tick. */
        std::uint32_t leaks = 0;
    };

    /**
     * @brief One level's fight: a level, its waves, its mobs and its
     * towers.
     *
     * Every number here is an integer and every decision is a pure
     * function of the tick count and the state, so a replay that
     * regenerates the same level regenerates the same battle.
     * Nothing is read from a clock and nothing is drawn from a global
     * generator -- the one draw a battle depends on, the order its waves
     * come out in, was made before it started and handed in.
     *
     * It keeps no score and no lives of its own: those span the levels
     * and belong to Campaign, which is what folds a step's outcome in.
     */
    class Battle final
    {
    public:
        /**
         * @brief Construct a battle over a generated level.
         * @param level The level to walk; copied, since the battle owns
         * the path its mobs index into.
         * @param config The numbers its guns are balanced with.
         * @param waves The waves to release, in order, each with its
         * spawn order already decided by planWaves().
         */
        Battle(
            Level level,
            BattleConfig config,
            std::vector<WaveRelease> waves);

        /**
         * @brief Try to put a tower on a cell.
         *
         * Refused on a cell that is part of the path, on a cell that
         * already holds a tower, and on a cell outside the grid.
         * A refusal is a return value rather than an exception: a click
         * landing somewhere unbuildable is ordinary input, not an error.
         *
         * @param cell Where to build.
         * @return True if a tower was built.
         */
        bool placeTower(const Cell &cell);

        /**
         * @brief Advance the battle by one tick.
         *
         * The order is fixed and load-bearing: mobs walk, then the wave
         * schedule may release one, then every tower fires.
         * Walking first means a mob that reaches the end this tick is
         * gone before anything can shoot it, and firing last means a mob
         * released this tick is already a target.
         *
         * @return What the tick earned and what it let through.
         */
        StepOutcome step();

        /**
         * @brief Whether every wave has been released and killed off.
         * @return True once nothing is left to fight.
         */
        [[nodiscard]] bool cleared() const;

        [[nodiscard]] const Level &level() const;
        [[nodiscard]] const BattleConfig &settings() const;
        [[nodiscard]] const std::vector<Mob> &mobs() const;
        [[nodiscard]] const std::vector<Tower> &towers() const;
        [[nodiscard]] std::uint64_t ticks() const;

        /** @brief How many waves this level has in total. */
        [[nodiscard]] std::size_t waveCount() const;

        /** @brief How many waves have been released in full. */
        [[nodiscard]] std::size_t wavesReleased() const;

        /**
         * @brief Take everything this battle has moved.
         * @return The fight, without the level it can rebuild.
         */
        [[nodiscard]] BattleMemory remember() const;

        /**
         * @brief Put a remembered fight back over this battle's level.
         *
         * Validated before anything moves, so a refused restore leaves
         * the battle exactly as it stood.
         *
         * @param memory What some battle's remember() answered.
         * @return True when it fit; false for a mob standing at or past
         * the end of this level's path, which is a dump for some other
         * level and is refused rather than repaired.
         */
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

} // namespace antwika::tower_defence
