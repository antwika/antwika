#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::replay::IReplaySource;

    /**
     * @brief Announces game startup and starts the engine.
     */
    class Game final
    {
    public:
        /**
         * @brief Construct the game over its engine and event dispatcher.
         * @param engine Engine started by run().
         * @param dispatcher Dispatcher used to announce the game is
         * running.
         */
        explicit Game(IEngine &engine, IEventDispatcher &dispatcher);

        Game(const Game &) = delete;
        Game(Game &&) = delete;

        Game &operator=(const Game &) = delete;
        Game &operator=(Game &&) = delete;

        /**
         * @brief Dispatch a startup event and start the engine.
         */
        void run();

    private:
        IEngine &engine;
        IEventDispatcher &dispatcher;
    };

    /**
     * @brief Wires the world, engine, event and replay collaborators
     * together, boots the game, then drives the tick loop until an
     * engine.stop event is dispatched.
     *
     * Sources each tick's events from inputSource -- clicks, scrolls and
     * drags, encoded by antwika::input -- until it dispatches engine.stop.
     * A live run and a loaded replay both use this same function; they
     * differ only in what inputSource was built from.
     *
     * Building the logger is the caller's job rather than this function's:
     * a composition root that also has to create a graphics backend needs
     * one before bootstrap() is ever called, and building a second one
     * here would put two loggers over one appender.
     *
     * @param logger Receives the run's diagnostics.
     * @param eventSink Receives every dispatched event.
     * @param inputSource Supplies each tick's events, live or replayed.
     * @param codec Decodes the input events off the tick stream.
     * @param extent Bounds which cells a click may reach.
     * @param camera Folded from input, and read by any renderer. Passed in
     * rather than created here because an observer built before this call
     * has to read it.
     * @param paths Recorded into as tiles are laid; likewise read by an
     * observer built beforehand.
     * @param observers Extra systems registered into an "observe" phase
     * that runs after "walk" every tick -- a renderer, a pacer. Defaults
     * to none, for callers that only need the final state.
     * @param maxTicks Optional safety cap on how many ticks to run before
     * giving up if engine.stop is never dispatched. Production callers can
     * leave this unset; tests should always pass one.
     * @param replayRecorder Optional sink that, if provided, receives every
     * dispatched event stamped with its tick -- what a caller wanting to
     * persist a `--record` file should register.
     * @return What the run amounted to, for callers and tests.
     * @throws antwika::replay::EngineLoopError If maxTicks is reached
     * without engine.stop.
     * @throws antwika::input::InputError If an input event carries a
     * payload of the wrong shape.
     */
    GameSummary bootstrap(
        ILogger &logger,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        const IInputEventCodec &codec,
        GridExtent extent,
        Camera &camera,
        PathIndex &paths,
        std::vector<std::reference_wrapper<ISystem>> observers = {},
        std::optional<antwika::time::Tick> maxTicks = std::nullopt,
        ITickEventSink *replayRecorder = nullptr);

} // namespace antwika::game
