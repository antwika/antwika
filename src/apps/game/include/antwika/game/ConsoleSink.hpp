#pragma once

#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/game/ConsoleScene.hpp"
#include "antwika/game/ConsoleState.hpp"
#include "antwika/game/InputFold.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/OptionsState.hpp"
#include "antwika/game/PauseState.hpp"
#include "antwika/game/SessionStore.hpp"
#include "antwika/game/UiOverlay.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::ui::Interactions;
    using antwika::ui::Keyboard;
    using antwika::ui::Pointer;

    /**
     * @brief Everything the console sink drives, named one per field.
     *
     * A struct with designated initialisers rather than a parameter
     * list, for the reason GameWiring gives: a dozen positional
     * references distinguishable only by where they sit is exactly how
     * two of them end up swapped.
     *
     * Every reference is borrowed and must outlive the ConsoleSink.
     */
    struct ConsoleSinkSetup
    {
        /** @brief The console being driven. */
        ConsoleState &console;

        /** @brief The run's bindings, for the toggle and execute keys. */
        const OptionsState &options;

        /** @brief The folded input, registered ahead of this sink. */
        const InputFold &input;

        /** @brief The console's own picture, written every tick. */
        UiOverlay &overlay;

        /** @brief Describes the console. */
        const ConsoleScene &scene;

        /** @brief Taken from by dump_state, put back by load_state. */
        SessionStore &session;

        /** @brief Carried by a dump, and restored by a load. */
        PauseState &pause;

        /** @brief Carried by a dump, and restored by a load. */
        MapViewState &view;

        /**
         * @brief The toolbar's shared state, for the selected tool.
         *
         * The one fact a dump takes off the bar: which tool a click
         * lays is simulation state, so coming back to an instant means
         * holding what it held.
         */
        UiOverlay &toolbar;

        /** @brief The run's language, staged back by a load. */
        LocaleState &locale;

        /**
         * @brief Whether load_state may run at all.
         *
         * False under --record and --replay, and the refusal is a
         * deterministic history line rather than a throw -- see
         * ConsoleSink for the rule it enforces.
         */
        bool loadEnabled = true;
    };

    /**
     * @brief Say whether a run's console may perform load_state.
     *
     * A function rather than an expression in main(), because a main
     * is branchless by rule and a && is a branch -- and because the
     * rule it states is worth a test: a load is permitted only in a
     * run that neither records nor replays.
     *
     * @param recording Whether --record was given.
     * @param replaying Whether --replay was given.
     * @return True only for a plain live run.
     */
    [[nodiscard]] bool consoleLoadPermitted(
        bool recording, bool replaying) noexcept;

    /**
     * @brief Turns this tick's input into the console's slide, its
     * typing and its commands, and the console into a picture.
     *
     * **The console defines no event of its own.** The toggle key, the
     * typing and the Enter that executes are the input; they are
     * resolved against the run's bindings and the console's own state
     * here, inside the tick path and downstream of the recorder, and
     * the slide, the history and the command's effect are all
     * regenerated from them on replay -- exactly as SaveLoadSink
     * regenerates a save from a click.
     * No `console.*` event name may ever exist.
     *
     * **dump_state runs everywhere, load_state only live.**
     * A dump is a write-only projection of state a replay reproduces,
     * so a replayed run re-executes it and rewrites the same file --
     * which is deliberate, and doubles as a way of reading any tick's
     * state out of a recording.
     * A load reads a file whose contents no recording carries, so a
     * recorded or replayed run answers it with a refusal line instead
     * -- the console-level twin of requireRecordableStart(), and the
     * refusal is itself deterministic, so a hand-authored replay that
     * types load_state reads exactly what a recorded run would have.
     *
     * Register it ahead of every sink that reads a key or a pixel --
     * the menu, the picker, the hotkeys, the toolbar, the world map
     * and the grid -- each of those wrapped in a ConsoleGatedSink.
     * The console is on top, so what it stands over it takes, and it
     * belongs to no mode: a debugging surface has to be reachable
     * from whichever screen the thing being debugged is on.
     */
    class ConsoleSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over everything it drives.
         * @param setup The collaborators, each of which must outlive
         * this sink.
         * @param dumpPath Where dump_state writes and load_state
         * reads, which is the one place this sink names a file --
         * a parameter beside the setup rather than a member of it,
         * on SaveLoadSink's exact shape.
         */
        ConsoleSink(const ConsoleSinkSetup &setup, std::string dumpPath);

        ConsoleSink(const ConsoleSink &) = delete;
        ConsoleSink(ConsoleSink &&) = delete;

        ConsoleSink &operator=(const ConsoleSink &) = delete;
        ConsoleSink &operator=(ConsoleSink &&) = delete;

        /**
         * @brief Apply a tick event.
         * @param event engine.tick advances the slide and re-describes
         * the picture; an input.* event is offered to the toggle key
         * and then, fully open, to the field; anything else is ignored.
         * @throws SaveFormatError If dump_state cannot write its file.
         * A full disk is the machine's truth rather than the run's, so
         * it ends the run instead of becoming a history line a replay
         * would then disagree about.
         */
        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(bool pressed, const Keyboard &keyboard);

        void act(const Interactions &interactions);

        void execute(const std::string &command);

        void dumpState();

        void loadState();

        ConsoleSinkSetup setup;
        std::string dumpPath;
    };

} // namespace antwika::game
