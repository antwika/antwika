#pragma once

#include <functional>
#include <optional>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/task_worker/PoolScene.hpp"
#include "antwika/task_worker/TaskRegistry.hpp"

namespace antwika::task_worker
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::gfx::IWindow;

    /**
     * @brief Draws the pool, the queue and this tick's budget into a
     * window, once per tick.
     *
     * The graphics counterpart to StatusPrintSystem, and an observer on
     * exactly the same terms: it reads the World and the TaskRegistry
     * and writes to neither, so it cannot change what the tick it is
     * drawing computed. Registered into the "observe" phase, which runs
     * after "dispatch", so the frame is of the state the tick ended
     * with.
     *
     * It lays out against the window's *configured* size rather than the
     * size the window reports. Nothing here is hit-tested, so that costs
     * nothing and buys the same picture on the machine that recorded a
     * session and on the one replaying it -- which is the rule
     * everywhere else in this project, kept here so that reading one
     * application does not teach the wrong thing.
     *
     * It never closes the window and asks it nothing else.
     */
    class RenderSystem final : public ISystem
    {
    public:
        /**
         * @brief Construct the system over everything it draws from.
         * @param window Window whose renderer receives each frame. Must
         * outlive this system.
         * @param scene Draws the pool. Must outlive this system.
         * @param registry Read for every task's status. Must outlive
         * this system.
         * @param consoleOverlay Read for the debug console's picture,
         * painted last, over the pool.
         * Described in the tick path like everything drawn here, and
         * painted only -- see antwika::console::ConsolePicture.
         * Optional, and absent by default, so a run with no console
         * paints exactly what it always did.
         */
        RenderSystem(
            IWindow &window,
            const PoolScene &scene,
            const TaskRegistry &registry,
            std::optional<std::reference_wrapper<
                const antwika::console::ConsolePicture>>
                consoleOverlay = std::nullopt);

        RenderSystem(const RenderSystem &) = delete;
        RenderSystem(RenderSystem &&) = delete;

        RenderSystem &operator=(const RenderSystem &) = delete;
        RenderSystem &operator=(RenderSystem &&) = delete;

        /**
         * @brief Snapshot this tick's pool and draw it.
         * @param world World read from; never written to.
         * @param tick The tick this frame is for, drawn as a caption.
         */
        void update(World &world, antwika::time::Tick tick) override;

    private:
        IWindow &window;
        const PoolScene &scene;
        const TaskRegistry &registry;
        std::optional<std::reference_wrapper<
            const antwika::console::ConsolePicture>>
            consoleOverlay;
    };

} // namespace antwika::task_worker
