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
#include <antwika/simulation/ITickSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/LevelGenerator.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickSource;

    /**
     * @brief What one run leaves behind, for a caller or a test.
     */
    struct BattleSummary
    {
        std::uint64_t score = 0;
        std::uint32_t leaks = 0;
        std::uint64_t ticks = 0;
        std::size_t towers = 0;
        std::size_t pathLength = 0;
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the battle
     * needs the Battle and the ScoreOverlay, and neither exists before
     * bootstrap() has generated a level.
     * Ownership passes back, so the sink lives exactly as long as the
     * run it belongs to.
     */
    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(const Battle &, const ScoreOverlay &)>;

    /**
     * @brief Everything one run is wired out of.
     *
     * A struct with designated initialisers rather than a parameter
     * list, so a wrong argument is a compile error rather than a
     * silently different run.
     */
    struct TowerDefenceConfig
    {
        /** @brief Receives the run's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /**
         * @brief The size everything is laid out and hit-tested against.
         *
         * The size the window was asked for, never the size one reports.
         */
        Size canvas;

        /** @brief Which level to generate; the seed lives here. */
        LevelConfig level = {};

        /** @brief The numbers the battle is balanced with. */
        BattleConfig battle = {};

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run gives up rather than
         * going on forever.
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
     * @brief Generate the level, wire the sinks up and run the loop.
     *
     * A live run and a replayed one are the same call: they differ only
     * in what inputSource was built from.
     *
     * @param config What the run is wired out of.
     * @return What the run ended on.
     * @throws LevelError If no level could be generated.
     */
    BattleSummary bootstrap(const TowerDefenceConfig &config);

} // namespace antwika::tower_defence
