#pragma once

#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>

#include "antwika/console/ConsoleGatedSink.hpp"
#include "antwika/console/ConsolePicture.hpp"
#include "antwika/console/ConsoleScene.hpp"
#include "antwika/console/ConsoleSink.hpp"
#include "antwika/console/ConsoleState.hpp"
#include "antwika/console/IConsoleControls.hpp"
#include "antwika/console/ISnapshotStore.hpp"
#include "antwika/console/InputFold.hpp"
#include "antwika/console/SnapshotCommands.hpp"

namespace antwika::console
{

    using antwika::event::ITickEventSink;

    /**
     * @brief What one application's console is mounted over, named
     * one per field.
     *
     * A struct with designated initialisers rather than a parameter
     * list, for ConsoleSinkSetup's reason: a row of same-typed
     * positional references is exactly how two of them end up
     * swapped.
     *
     * Every reference is borrowed and must outlive the ConsoleMount.
     */
    struct ConsoleMountSetup
    {
        /**
         * @brief The console's own picture, which turns it on.
         *
         * Unset is a run with no console at all: nothing is written
         * anywhere and ConsoleMount::mounted() answers false, so the
         * application registers no sink and the toggle key stays a
         * plain unbound key.
         */
        std::optional<std::reference_wrapper<ConsolePicture>> overlay;

        /**
         * @brief The fold every console-mounting run registers first.
         *
         * Borrowed rather than owned, because an application whose own
         * sinks read input folds it once for all of them and hands the
         * same fold in here.
         */
        InputFold &input;

        /** @brief The application's half of the snapshot seam. */
        ISnapshotStore &store;

        /** @brief Where dump_state writes and load_state reads. */
        const std::string &dumpPath;

        /**
         * @brief Whether load_state may run at all.
         *
         * False under --record and --replay -- see
         * consoleLoadPermitted().
         */
        bool loadEnabled;

        /**
         * @brief The toggle key, the execute key and the board.
         *
         * Unset is the shipped constants, which is what an
         * application with no options screen to rebind them on wants;
         * the game answers off its own rebindable options instead.
         */
        std::optional<std::reference_wrapper<const IConsoleControls>>
            controls = std::nullopt;
    };

    /**
     * @brief One application's whole console, mounted in one object.
     *
     * The picture, the state, the scene, the controls, the snapshot
     * commands and the ConsoleSink over them: the block every
     * application's bootstrap used to spell out for itself, in
     * comments included, and which drifted between them exactly as a
     * copied block does.
     *
     * It registers nothing.
     * Where each sink goes in a run's list is a correctness contract
     * rather than a detail to hide -- the fold first, this sink ahead
     * of everything it gates, and the recorder where the application
     * put it -- so the bootstrap goes on writing its own list and asks
     * this only for the sink to put in it.
     *
     * **No console means no console, not an invisible one.**
     * With no overlay in the setup there is nowhere to put a picture,
     * so mounted() answers false and the application registers no
     * console sink at all; the state then stays closed for the whole
     * run and every gate() forwards everything, untouched.
     */
    class ConsoleMount final
    {
    public:
        /**
         * @brief Mount a console over everything it drives.
         * @param setup The collaborators, each of which must outlive
         * this object.
         */
        explicit ConsoleMount(const ConsoleMountSetup &setup);

        ConsoleMount(const ConsoleMount &) = delete;
        ConsoleMount(ConsoleMount &&) = delete;

        ConsoleMount &operator=(const ConsoleMount &) = delete;
        ConsoleMount &operator=(ConsoleMount &&) = delete;

        /**
         * @brief Check whether this run has a console at all.
         * @return True only when the setup carried an overlay, which
         * is the one condition under which sink() may be registered.
         */
        [[nodiscard]] bool mounted() const noexcept;

        /**
         * @brief Get the sink that drives the console.
         *
         * Register it only while mounted(), and ahead of every sink
         * it gates.
         *
         * @return The sink, for the run's own list of them.
         */
        [[nodiscard]] ConsoleSink &sink() noexcept;

        /**
         * @brief Get the console's own simulation state.
         *
         * The slide, the field and the history, all folded inside the
         * tick path -- which is where a summary reads its history out
         * of on the way out.
         *
         * @return The state this mount drives.
         */
        [[nodiscard]] ConsoleState &state() noexcept;

        /**
         * @brief Wrap a sink in this console's gate.
         *
         * "The console covers this" said once, where the sink is
         * registered: what the console stands over, it takes.
         *
         * @param inner The sink to forward to. Must outlive the gate.
         * @return The gate, which borrows this mount and must not
         * outlive it.
         */
        [[nodiscard]] ConsoleGatedSink gate(
            ITickEventSink &inner) const noexcept;

    private:
        ConsolePicture noConsole;
        bool isMounted;
        ConsolePicture &picture;
        InputFold &input;
        ConsoleState console;
        const ConsoleScene scene{};
        const FixedConsoleControls fixedControls;
        const IConsoleControls &controls;
        SnapshotCommands commands;
        ConsoleSink consoleSink;
    };

} // namespace antwika::console
