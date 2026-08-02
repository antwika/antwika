#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/game/AppMode.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/RoadDrag.hpp"
#include "antwika/game/SaveGame.hpp"
#include "antwika/game/UiCanvas.hpp"
#include "antwika/game/UiOverlay.hpp"
#include "antwika/game/WorldMapState.hpp"

namespace antwika::game
{

    using antwika::ecs::ISystem;
    using antwika::engine::IEngine;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickEventSource;

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
        ITickEventSource &inputSource;

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
         * @brief Which cells hold a building, as of right now.
         *
         * Shared rather than owned by the sink that writes it, because
         * BuildingSystem clears a cell as it demolishes one and
         * GridSink records a cell as it builds on one -- and a note
         * either kept to itself would drift from the other.
         */
        BuildingIndex &built;

        /**
         * @brief Which screen the run is on, folded by the tick path.
         *
         * Passed in rather than created here for the same reason the
         * camera is: a renderer built before this call has to read it to
         * know which mode's picture to draw.
         *
         * Its constructor is where a run says which mode it starts in.
         * The application leaves that defaulted, so a session opens at
         * the main menu; a test whose subject is the grid constructs one
         * as AppMode::CityMap rather than clicking its way there.
         */
        AppModeState &mode;

        /**
         * @brief Whether the run is held still, toggled by the tick path.
         *
         * Passed in rather than created here for the camera's reason
         * again: a renderer built before this call has to read it, since
         * a walker slides between two cells over frames a pause does not
         * stop -- see SceneSnapshot::paused.
         *
         * A fresh one is unpaused, and nothing here ever pauses one:
         * a run progresses all the time until a player asks the bar's
         * button for a pause -- see PauseState.
         */
        PauseState &pause;

        /**
         * @brief Where a run of road is being dragged out, if anything
         * draws one.
         *
         * Optional for the toolbar's reason rather than the pause's: a
         * run with nobody to show a preview to still drags roads out
         * perfectly well, and one of its own is made here when no caller
         * offers one. A caller that draws the preview has to own it, so
         * that the renderer it built first can read it.
         */
        std::optional<std::reference_wrapper<RoadDrag>> drag = std::nullopt;

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
         * @brief The main menu's own picture, which turns the menu on.
         *
         * A second overlay rather than the toolbar's, because the two
         * belong to different modes and neither may overwrite the
         * other's picture.
         *
         * Unset, the menu is described against a zero canvas, which no
         * click can hit -- so a run that starts in AppMode::MainMenu
         * without one never leaves it. Every caller starting at the menu
         * must therefore set this; one starting in AppMode::CityMap need
         * not, and a test whose subject is the grid does not.
         *
         * Passed in rather than created here because a renderer built
         * beforehand has to read it.
         */
        std::optional<std::reference_wrapper<UiOverlay>> menuOverlay =
            std::nullopt;

        /**
         * @brief The world and its cities, which turns the world map on.
         *
         * Set, a WorldMapSink is registered ahead of the grid's, so a
         * press on a city opens it and the way-back key puts it away.
         * The grid the session builds on is swapped in and out of this
         * as cities are opened -- see WorldMapState.
         *
         * Unset, the run has one city and one grid, which is what every
         * caller whose subject is the grid wants: bootstrap() then keeps
         * a world of its own with city 0 permanently open, so nothing is
         * gated off by a world map that is not there.
         *
         * Passed in rather than created here because a renderer built
         * beforehand has to read it, and because generating a world is
         * the composition root's decision -- it is what owns the seed.
         */
        std::optional<std::reference_wrapper<WorldMapState>> world =
            std::nullopt;

        /**
         * @brief The save/load screen's own picture, which turns it on.
         *
         * A third overlay rather than the menu's or the toolbar's, for
         * the reason the menu has one of its own: the three belong to
         * different modes and none may overwrite another's picture.
         *
         * Unset, the screen is described against a zero canvas, which no
         * click can hit -- so a run that reaches AppMode::SaveLoad
         * without one never leaves it. Every caller offering the menu's
         * "Load Game" must therefore set this.
         *
         * Passed in rather than created here because a renderer built
         * beforehand has to read it.
         */
        std::optional<std::reference_wrapper<UiOverlay>> saveOverlay =
            std::nullopt;

        /**
         * @brief The saves that existed when the run started.
         *
         * Read once, before the loop, and fixed for the run -- see
         * listSaveGames() for why a directory may not be read from
         * inside the tick path.
         */
        std::vector<std::string> saves = {};

        /**
         * @brief Where the save/load screen writes and reads.
         */
        std::string saveDirectory = {};

        /**
         * @brief The state to resume from, if `--load` named one.
         *
         * Restored through the same SessionStore the Load button uses,
         * before the first tick, so a session resumed from the command
         * line and one resumed mid-run cannot come out differently.
         */
        std::optional<SaveGame> start = std::nullopt;

        /**
         * @brief Where to write the session as the run ends, if anywhere.
         *
         * Written from in here rather than by the caller, because a save
         * is taken from the World and the World does not outlive this
         * call.
         * It is the mirror of `start`, which loads through the same
         * store the Load button uses.
         */
        std::optional<std::string> savePath = std::nullopt;

        /**
         * @brief Where this run should leave the key bindings it ends
         * with, if anywhere.
         *
         * The mirror of savePath, and written from in here for the same
         * reason: the bindings are taken off state this call owns, and
         * that state does not outlive it.
         *
         * Unset writes nowhere, which is what every test wants and what
         * a --replay run gets: a run that was not allowed to read the
         * machine's layout may not overwrite it either -- see
         * machineOptionsFor().
         */
        std::optional<std::string> optionsPath = std::nullopt;

        /**
         * @brief The seed every generated part of the session came from.
         *
         * Written into a save so that a resumed session regenerates the
         * same world -- see SaveGame::seed.
         */
        std::uint64_t seed = 0;

        /**
         * @brief The language every caption in the run is worded in.
         *
         * **Injected rather than reached for, and fixed in source.** A
         * layout is a function of the strings declared into it and a
         * hit-test is a function of the layout, so a run recorded in one
         * language and replayed in another would resolve one recorded
         * click to two different widgets -- see Translator.hpp.
         *
         * Unset, one at kDefaultLocale is made here. That is not a
         * default anybody may vary: it is the same fixed-in-source
         * choice a composition root makes, written once so a test whose
         * subject is the grid need not say it.
         *
         * Passed in rather than created here because a renderer built
         * beforehand words its own scenes with it.
         */
        std::optional<
            std::reference_wrapper<const antwika::i18n::Translator>>
            translator = std::nullopt;

        /**
         * @brief The area every mode's UI is laid out against.
         *
         * The size the window was *asked* for, which the world map is
         * centred in and which a click on a city is resolved against.
         * Defaulted to kUiCanvas, the one number the shipped app uses.
         */
        Size canvas = kUiCanvas;
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
     * @throws antwika::simulation::EngineLoopError If maxTicks is reached
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
