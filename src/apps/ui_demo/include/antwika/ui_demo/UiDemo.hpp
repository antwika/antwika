#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/ui_demo/DemoOverlay.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/Showcase.hpp"

namespace antwika::ui_demo
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief What one run leaves behind, for a caller or a test.
     */
    struct DemoSummary
    {
        /** @brief The page that was showing when the run ended. */
        Showcase showcase = Showcase::Labels;

        /** @brief How many times the counting button was pressed. */
        std::uint32_t clicks = 0;

        /** @brief How many commands the last frame drew. */
        std::size_t commands = 0;
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the
     * showcase needs the DemoOverlay, and that does not exist before
     * bootstrap() has made one.
     * Ownership passes back, so the sink lives exactly as long as the
     * run it belongs to.
     */
    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(const DemoState &,
                                        const DemoOverlay &)>;

    /**
     * @brief Everything one run is wired out of.
     *
     * A struct with designated initialisers rather than a parameter
     * list, so a wrong argument is a compile error rather than a
     * silently different run.
     */
    struct UiDemoConfig
    {
        /** @brief Receives the run's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickEventSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /**
         * @brief The size everything is laid out and hit-tested against.
         *
         * The size the window was asked for, never the size one reports.
         */
        Size canvas;

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run throws rather than going
         * on forever; the demo itself ends through TickBudgetSource,
         * which is an ordinary stop.
         * Tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /** @brief Sink receiving every dispatched event, tick-stamped. */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /** @brief Factory for one more tick sink, e.g. the renderer. */
        TickSinkFactory extraSink = {};
    };

    /**
     * @brief Wire the sinks up and run the loop.
     *
     * A live run and a replayed one are the same call: they differ only
     * in what inputSource was built from.
     *
     * @param config What the run is wired out of.
     * @return What the run ended on.
     * @throws antwika::simulation::EngineLoopError If maxTicks is reached
     * without an engine.stop.
     */
    DemoSummary bootstrap(const UiDemoConfig &config);

} // namespace antwika::ui_demo
