#pragma once

#include <cstdint>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/BoardScene.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::IWindow;

    /**
     * @brief Draws every Cell's alive state into a window, once per tick.
     *
     * The graphics counterpart to PrintSystem, and an observer on the same
     * terms: it only reads World, never writes it, and knows nothing about
     * LifeSystem or any other system in the same "observe" phase. Because
     * SystemScheduler commits after every phase, what it draws is the
     * generation LifeSystem produced during this tick -- the seeded state
     * is never itself a frame.
     *
     * It never closes the window, and does not check whether the window is
     * still open. Window lifetime belongs to the composition root that
     * created it, which keeps it open for the whole run (see main.cpp),
     * and a backend never closes a window on its own.
     *
     * The window's size is read afresh every tick, so a resize needs no
     * handling of its own. That size reaches nothing but IRenderer calls,
     * which is what keeps a resize from perturbing the simulation.
     */
    class RenderSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over the window it draws into.
         * @param window Window whose renderer receives each frame.
         * Must outlive this system.
         * @param scene Draws the board. Must outlive this system.
         * @param width Number of columns on the board.
         * @param height Number of rows on the board.
         */
        RenderSystem(
            IWindow &window,
            const BoardScene &scene,
            std::uint32_t width,
            std::uint32_t height);

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;
        RenderSystem &operator=(RenderSystem &&) = delete;

        /**
         * @brief Draw World's current Cell states and present the frame.
         * @param world World read from; never written to.
         * @param tick The tick this frame is for; unused.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        IWindow &window;
        const BoardScene &scene;
        std::uint32_t width;
        std::uint32_t height;
    };

} // namespace antwika::life
