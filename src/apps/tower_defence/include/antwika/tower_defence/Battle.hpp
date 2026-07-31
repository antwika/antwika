#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Level.hpp"

namespace antwika::tower_defence
{

    /** @brief One walker, advancing one path cell per tick. */
    struct Mob
    {
        std::uint32_t id = 0;

        /** @brief How far along Level::path it has walked. */
        std::size_t pathIndex = 0;

        std::int32_t health = 0;
    };

    /** @brief One gun, sitting on a cell no mob will ever walk. */
    struct Tower
    {
        std::uint32_t id = 0;
        Cell cell;
    };

    /** @brief The numbers one battle is balanced with. */
    struct BattleConfig
    {
        /** @brief A mob spawns every this many ticks, first at tick 0. */
        std::uint64_t spawnPeriodTicks = 6;

        /** @brief Health each mob spawns with. */
        std::int32_t mobHealth = 3;

        /**
         * @brief Squared reach of a tower, in cells.
         *
         * Squared so the check is integer throughout: no sqrt, and so no
         * floating point anywhere in the simulation.
         * The default is a radius of two cells, which is deliberately
         * short -- a tower covers the corner it is put on and little
         * more, so where it goes is the whole decision.
         */
        std::uint32_t towerRangeSquared = 4;

        /** @brief Health a tower takes off its target each tick. */
        std::int32_t towerDamage = 1;

        /** @brief Score for killing a mob. */
        std::uint64_t killScore = 10;

        /** @brief Score lost when a mob reaches the end. */
        std::uint64_t leakPenalty = 25;
    };

    /**
     * @brief The whole simulation: a level, its mobs, its towers, a
     * score.
     *
     * Every number here is an integer and every decision is a pure
     * function of the tick count and the state, so a replay that
     * regenerates the same level regenerates the same battle.
     * Nothing is read from a clock and nothing is drawn from a global
     * generator.
     */
    class Battle final
    {
    public:
        /**
         * @brief Construct a battle over a generated level.
         * @param level The level to walk; copied, since the battle owns
         * the path its mobs index into.
         * @param config The numbers to balance it with.
         */
        Battle(Level level, BattleConfig config);

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
         * The order is fixed and load-bearing: mobs walk, then a mob may
         * spawn, then every tower fires.
         * Walking first means a mob that reaches the end this tick is
         * gone before anything can shoot it, and firing last means a mob
         * spawned this tick is already a target.
         */
        void step();

        [[nodiscard]] const Level &level() const;
        [[nodiscard]] const std::vector<Mob> &mobs() const;
        [[nodiscard]] const std::vector<Tower> &towers() const;
        [[nodiscard]] std::uint64_t score() const;
        [[nodiscard]] std::uint64_t ticks() const;

        /** @brief How many mobs have reached the end. */
        [[nodiscard]] std::uint32_t leaks() const;

    private:
        Level levelData;
        BattleConfig config;
        std::vector<Mob> living;
        std::vector<Tower> guns;
        std::uint64_t tickCount = 0;
        std::uint64_t totalScore = 0;
        std::uint32_t leaked = 0;
        std::uint32_t nextMobId = 0;
        std::uint32_t nextTowerId = 0;

        void walk();
        void spawn();
        void fire();
    };

} // namespace antwika::tower_defence
