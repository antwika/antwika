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
#include <antwika/simulation/ITickSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/DragState.hpp"
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
    using antwika::simulation::ITickSource;

    /**
     * @brief Pick the systems that watch each tick of a run.
     *
     * A backend that draws nothing leaves the board to be printed, and
     * either way the run is paced -- last, after the frame, since a run
     * with no end of its own would otherwise go flat out.
     *
     * It is a function rather than three lines in a main() because a
     * composition root is excluded from the coverage report, and which
     * systems observe a headless run is not a thing to leave unmeasured.
     *
     * @param renderer Draws the board each tick.
     * @param printer Writes the board out, for a run nobody can watch.
     * @param pacer Holds each tick back to the wall clock.
     * @param drawsNothing Whether the backend shows anything at all.
     * @return The observers, in the order they should run.
     */
    [[nodiscard]] std::vector<std::reference_wrapper<ISystem>> observersFor(
        ISystem &renderer,
        ISystem &printer,
        ISystem &pacer,
        bool drawsNothing);

    /**
     * @brief Say how to stop a run nobody can close a window on.
     *
     * The run has no end of its own: it goes on until the window closes,
     * or until a replay says to stop. A headless build reports neither.
     *
     * @param logger Where the notice is written.
     * @param drawsNothing Whether the backend shows anything at all.
     */
    void announceHowToStop(ILogger &logger, bool drawsNothing);

    /**
     * @brief Builds a tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that folds events into
     * the board needs the World, the Grid and the DragState, and none of
     * them exists before bootstrap() creates them. Ownership passes back,
     * so the sink lives exactly as long as the run it belongs to.
     */
    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(World &, const Grid &, DragState &)>;

    /**
     * @brief Announces the run in the log and starts the engine.
     *
     * The announcement is a log line rather than an event, because
     * nothing consumes it: as an event, every app dispatched one and then
     * stripped it by name again before writing a recording, since
     * persisting it would make a replay dispatch it twice.
     */
    class Life final
    {
    public:
        /**
         * @brief Construct the simulation over its engine and logger.
         * @param engine Engine started by run().
         * @param logger Receives the announcement that it is running.
         */
        explicit Life(IEngine &engine, ILogger &logger);

        Life(const Life &) = delete;
        Life(Life &&) = delete;

        Life &operator=(const Life &) = delete;
        Life &operator=(Life &&) = delete;

        /**
         * @brief Log that the simulation is running and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    /**
     * @brief Everything one run of the simulation is wired out of.
     *
     * A struct with designated initialisers rather than a parameter list,
     * because the list had reached nine positional arguments, two of them
     * unlabelled numbers next to each other and one a nullable pointer.
     * A name per argument is what makes a wrong one a compile error
     * rather than a silently different run.
     */
    struct LifeConfig
    {
        /** @brief Receives the run's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickSource &inputSource;

        /** @brief Number of columns in the board. */
        std::uint32_t width;

        /** @brief Number of rows in the board. */
        std::uint32_t height;

        /**
         * @brief Extra systems registered into an "observe" phase.
         *
         * The phase runs after "life" every tick, and each system is
         * fully independent of both LifeSystem and each other (e.g.
         * PrintSystem). Empty for callers that only need the final Board.
         */
        std::vector<std::reference_wrapper<ISystem>> observers = {};

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run gives up rather than going
         * on forever. Production callers can leave this unset to run
         * uncapped; tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /**
         * @brief Sink receiving every dispatched event, stamped with its
         * tick.
         *
         * What a caller wanting to persist a `--record` file registers,
         * since a run's actual length is not known ahead of time.
         */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /**
         * @brief Factory for one more tick sink.
         *
         * Called once with the World, Grid and DragState bootstrap()
         * owns. A sink folding events into the board needs all three, and
         * none of them exists until then -- what main.cpp uses to add
         * PointerToggleSink.
         */
        TickSinkFactory extraSink = {};
    };

    /**
     * @brief Wires the ECS world, engine, event, and replay collaborators
     * together, boots the simulation, then drives the tick loop until an
     * engine.stop event is dispatched.
     *
     * Sources each tick's events from the config's inputSource --
     * typically events::kToggleCell, seeding the initial pattern -- until
     * it dispatches engine.stop. A hand-scripted "live" run and a loaded
     * replay both use this same function; they differ only in what
     * inputSource was built from, the same contract apps/game's
     * bootstrap() follows for its own state.
     *
     * Building the logger is the caller's job, not this function's -- a
     * composition root that also has to open a window needs one before
     * bootstrap() is ever called (see main.cpp).
     *
     * @param config What the run is wired out of.
     * @return The resulting Board, for callers (main.cpp, tests).
     */
    Board bootstrap(const LifeConfig &config);

} // namespace antwika::life
