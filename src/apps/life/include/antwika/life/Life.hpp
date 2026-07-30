#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/Grid.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::log::ILogger;
    using antwika::replay::IReplaySource;

    /**
     * @brief Builds a tick sink over the World and Grid bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that folds events into
     * the board needs both of those, and neither exists before
     * bootstrap() creates them. Ownership passes back, so the sink lives
     * exactly as long as the run it belongs to.
     */
    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(World &, const Grid &)>;

    /**
     * @brief Announces simulation startup and starts the engine.
     */
    class Life final
    {
    public:
        /**
         * @brief Construct the simulation over its engine and dispatcher.
         * @param engine Engine started by run().
         * @param dispatcher Dispatcher used to announce startup.
         */
        explicit Life(IEngine &engine, IEventDispatcher &dispatcher);

        Life(const Life &) = delete;
        Life(Life &&) = delete;

        Life &operator=(const Life &) = delete;
        Life &operator=(Life &&) = delete;

        /**
         * @brief Dispatch a startup event and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    /**
     * @brief Wires the ECS world, engine, event, and replay collaborators
     * together, boots the simulation, then drives the tick loop until an
     * engine.stop event is dispatched.
     *
     * Sources each tick's events from inputSource -- typically
     * events::kToggleCell, seeding the initial pattern -- until it
     * dispatches engine.stop. A hand-scripted "live" run and a loaded
     * replay both use this same function; they differ only in what
     * inputSource was built from, the same contract apps/game's
     * bootstrap() follows for its own state.
     *
     * Building the logger is the caller's job, not this function's -- a
     * composition root that also has to open a window needs one before
     * bootstrap() is ever called (see main.cpp).
     *
     * @param logger Receives the run's diagnostics.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param width Number of columns in the board.
     * @param height Number of rows in the board.
     * @param observers Extra systems registered into an "observe" phase
     * that runs after "life" every tick -- each is fully independent of
     * both LifeSystem and each other (e.g. PrintSystem). Defaults to
     * none, for callers (like the tests) that only need the final Board.
     * @param maxTicks Optional safety cap on how many ticks to run before
     * giving up if engine.stop is never dispatched. Production callers
     * can leave this unset to run uncapped; tests should always pass one.
     * @param replayRecorder Optional sink that, if provided, receives
     * every dispatched event stamped with its tick -- what a caller
     * wanting to persist a `--record` file should register, since a run's
     * actual length is no longer known ahead of time. Defaults to none.
     * @param extraSink Optional factory for one more tick sink, called
     * once with the World and Grid this function owns. A sink folding
     * events into the board needs both, and neither exists until here --
     * what main.cpp uses to add PointerToggleSink. Defaults to none.
     * @return The resulting Board, for callers (main.cpp, tests).
     */
    Board bootstrap(
        ILogger &logger,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::uint32_t width,
        std::uint32_t height,
        std::vector<std::reference_wrapper<ISystem>> observers = {},
        std::optional<antwika::time::Tick> maxTicks = std::nullopt,
        ITickEventSink *replayRecorder = nullptr,
        const TickSinkFactory &extraSink = {});

} // namespace antwika::life
