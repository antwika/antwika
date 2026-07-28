#pragma once

#include <cstdint>
#include <ostream>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;

    /**
     * @brief Prints every Cell's alive state as ASCII art, once per tick.
     *
     * An independent, ECS-shaped per-tick observer -- it only reads World
     * (via world.view<Cell>()), never writes it, and knows nothing about
     * LifeSystem or any other system registered alongside it in the same
     * "observe" phase, so any number of such observers can coexist.
     *
     * Relies on Grid creating cells in row-major order and
     * ComponentStorage's insertion-order stability (see antwika::ecs) to
     * read world.view<Cell>() back out in that same row-major order,
     * without needing a Grid reference of its own.
     */
    class PrintSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the board width and output.
         * @param width Number of cells per printed row.
         * @param out Stream every board snapshot is printed to.
         * Must outlive this system.
         */
        PrintSystem(std::uint32_t width, std::ostream &out);

        /**
         * @brief Print World's current Cell states as ASCII art.
         * @param world World read from; never written to.
         * @param tick The tick this snapshot is for, printed as a label.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        std::uint32_t width;
        std::ostream &out;
    };

} // namespace antwika::life
