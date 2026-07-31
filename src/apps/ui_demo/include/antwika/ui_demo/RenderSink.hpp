#pragma once

#include <chrono>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/time/ISleeper.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoScene.hpp"

namespace antwika::ui_demo
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::IWindow;
    using antwika::time::ISleeper;

    /**
     * @brief Draws the showcase, once per engine.tick.
     *
     * Rendering hangs off the tick loop without feeding back into it:
     * everything it draws arrives as a DrawList somebody else described,
     * and nothing it does is visible to any other sink.
     * Registered after DemoSink, so a frame is of the state the tick
     * ended with.
     *
     * It never closes the window and never asks it anything but whether
     * it is still open.
     */
    class RenderSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it draws from.
         * @param window Window whose renderer receives each frame. Must
         * outlive this sink.
         * @param scene Draws the picture. Must outlive this sink.
         * @param overlay Holds the picture. Must outlive this sink.
         * @param sleeper Paces the frames. Must outlive this sink.
         * @param framePeriod How long to hold each frame.
         */
        RenderSink(
            IWindow &window,
            const DemoScene &scene,
            const DemoOverlay &overlay,
            ISleeper &sleeper,
            std::chrono::milliseconds framePeriod);

        RenderSink(const RenderSink &) = delete;
        RenderSink(RenderSink &&) = delete;

        RenderSink &operator=(const RenderSink &) = delete;
        RenderSink &operator=(RenderSink &&) = delete;

        /**
         * @brief Draw a frame if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        IWindow &window;
        const DemoScene &scene;
        const DemoOverlay &overlay;
        ISleeper &sleeper;
        std::chrono::milliseconds framePeriod;
    };

} // namespace antwika::ui_demo
