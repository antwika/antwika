#pragma once

#include <cstdint>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Applies Conway's Game of Life rule to every cell, once per tick.
     *
     * Every cell's next value is computed purely from World's front buffer
     * (the board as of the last commit), and staged into the back buffer
     * via World::set. The double buffering World already provides is what
     * keeps every cell's neighbor count immune to any other cell's
     * already-computed next value within the same tick.
     */
    class LifeSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the grid it operates on.
         * @param grid Maps (x, y) coordinates to entities.
         * Must outlive this system.
         */
        explicit LifeSystem(const Grid &grid);

        LifeSystem(const LifeSystem &) = delete;
        LifeSystem(LifeSystem &&) = delete;

        LifeSystem &operator=(const LifeSystem &) = delete;
        LifeSystem &operator=(LifeSystem &&) = delete;

        /**
         * @brief Advance every cell in the grid by one generation.
         * @param world World read from and staged into.
         * @param tick Unused -- the rule depends only on World's current
         * front buffer, not on which tick it's being applied for.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        [[nodiscard]] int countAliveNeighbors(
            const World &world, std::uint32_t x, std::uint32_t y) const;

        const Grid &grid;
    };

} // namespace antwika::life
