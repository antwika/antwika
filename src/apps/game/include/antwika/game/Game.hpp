#pragma once

#include <functional>
#include <optional>
#include <ostream>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/MenuState.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::replay::IReplaySource;

    /**
     * @brief Announces the run in the log and starts the engine.
     *
     * The announcement is a log line rather than an event, because
     * nothing consumes it: as an event, every app dispatched one and then
     * stripped it by name again before writing a recording, since
     * persisting it would make a replay dispatch it twice.
     */
    class Game final
    {
    public:
        /**
         * @brief Construct the game over its engine and logger.
         * @param engine Engine started by run().
         * @param logger Receives the announcement that the game is
         * running.
         */
        explicit Game(IEngine &engine, ILogger &logger);

        Game(const Game &) = delete;
        Game(Game &&) = delete;

        Game &operator=(const Game &) = delete;
        Game &operator=(Game &&) = delete;

        /**
         * @brief Log that the game is running and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    /**
     * @brief Everything one run of the game is wired out of.
     *
     * A struct with designated initialisers rather than a parameter list,
     * because the list had grown to eleven positional arguments, two of
     * them raw pointers distinguishable only by position and one of them
     * a bare std::nullopt whose meaning was invisible at the call site.
     * A name per argument is what makes a wrong one a compile error
     * rather than a silently different run.
     *
     * The optional collaborators are std::optional<reference_wrapper>
     * rather than pointers: absent means absent, and there is no third
     * state where a caller passed something that is not there.
     */
    struct GameConfig
    {
        /** @brief Receives the run's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        IReplaySource &inputSource;

        /** @brief Decodes the input events off the tick stream. */
        const IInputEventCodec &codec;

        /** @brief Bounds which cells a click may reach. */
        GridExtent extent;

        /**
         * @brief Folded from input, and read by any renderer.
         *
         * Passed in rather than created here because an observer built
         * before the call has to read it.
         */
        Camera &camera;

        /**
         * @brief Recorded into as tiles are laid.
         *
         * Likewise read by an observer built beforehand.
         */
        PathIndex &paths;

        /**
         * @brief Extra systems registered into an "observe" phase.
         *
         * The phase runs after "walk" every tick -- a renderer, a pacer.
         * Empty for callers that only need the final state.
         */
        std::vector<std::reference_wrapper<ISystem>> observers = {};

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run gives up rather than going
         * on forever. Production callers can leave this unset; tests
         * should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /**
         * @brief Sink receiving every dispatched event, stamped with its
         * tick.
         *
         * What a caller wanting to persist a `--record` file registers.
         */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /**
         * @brief Shared UI state, which turns the toolbar on.
         *
         * Set, a UiSink is registered ahead of the grid's, describing and
         * resolving the bar against the overlay's canvas -- see
         * kUiCanvas for why that canvas is the size the window was asked
         * for. Unset, no UiSink is registered at all, so the run has no
         * toolbar and every click is the world's.
         *
         * Passed in rather than created here because a renderer built
         * beforehand has to read it.
         */
        std::optional<std::reference_wrapper<UiOverlay>> overlay =
            std::nullopt;

        /**
         * @brief The main menu's state, opened and closed by F10.
         *
         * Passed in rather than created here because the caller is what
         * acts on MenuState::activated: this library opens no file
         * dialog, so loading and saving are reported as intents and the
         * composition root does something with them -- see MenuSink.
         *
         * Unset, a menu is still wired in over the toolbar and F10 still
         * works; the state is simply one nothing outside can read. What
         * decides whether there is a menu at all is the overlay, since a
         * menu with no canvas would cover nothing and hit-test nothing.
         */
        std::optional<std::reference_wrapper<MenuState>> menuState =
            std::nullopt;
    };

    /**
     * @brief Wires the world, engine, event and replay collaborators
     * together, boots the game, then drives the tick loop until an
     * engine.stop event is dispatched.
     *
     * Sources each tick's events from the config's inputSource -- clicks,
     * scrolls and drags, encoded by antwika::input -- until it dispatches
     * engine.stop. A live run and a loaded replay both use this same
     * function; they differ only in what inputSource was built from.
     *
     * Building the logger is the caller's job rather than this function's:
     * a composition root that also has to create a graphics backend needs
     * one before bootstrap() is ever called, and building a second one
     * here would put two loggers over one appender.
     *
     * @param config What the run is wired out of.
     * @return What the run amounted to, for callers and tests.
     * @throws antwika::replay::EngineLoopError If maxTicks is reached
     * without engine.stop.
     * @throws antwika::input::InputError If an input event carries a
     * payload of the wrong shape.
     */
    GameSummary bootstrap(const GameConfig &config);

    /**
     * @brief Write what a run amounted to.
     *
     * It lives here rather than in a main() so that a test can read it:
     * a composition root is excluded from the coverage report, and a
     * loop over the walkers is exactly what that exclusion should not be
     * hiding.
     *
     * @param out Where the summary is written.
     * @param summary What the run amounted to.
     */
    void printSummary(std::ostream &out, const GameSummary &summary);

} // namespace antwika::game
