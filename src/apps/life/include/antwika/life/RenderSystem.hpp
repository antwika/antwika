#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include <antwika/console/ConsolePicture.hpp>
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
     * **The board is drawn against the size the window was asked for,
     * never the size it reports.** PointerToggleSink lays the same board
     * out from the configured size to work out which cell a press
     * landed in, and the two have to be one function or a window manager
     * handing back a size of its own choosing would put a cell somewhere
     * other than where it is clicked -- see docs/resizable-windows.md.
     * It is read afresh every tick rather than kept, so this system
     * holds no copy of a number the window already owns.
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
         * @param console The debug console's picture, painted over the
         * board; absent for a run with no console. Must outlive this
         * system when present.
         */
        RenderSystem(
            IWindow &window,
            const BoardScene &scene,
            std::uint32_t width,
            std::uint32_t height,
            std::optional<std::reference_wrapper<
                const antwika::console::ConsolePicture>>
                console = std::nullopt);

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
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            console;
    };

} // namespace antwika::life
