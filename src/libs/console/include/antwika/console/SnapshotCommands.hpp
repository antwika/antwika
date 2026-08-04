#pragma once

#include <string>

#include "antwika/console/IConsoleCommands.hpp"
#include "antwika/console/ISnapshotStore.hpp"

namespace antwika::console
{

    /**
     * @brief Say whether a run's console may perform load_state.
     *
     * A function rather than an expression in a main(), because a
     * main is branchless by rule and a && is a branch -- and because
     * the rule it states is worth a test: a load is permitted only in
     * a run that neither records nor replays.
     *
     * @param recording Whether --record was given.
     * @param replaying Whether --replay was given.
     * @return True only for a plain live run.
     */
    [[nodiscard]] bool consoleLoadPermitted(
        bool recording, bool replaying) noexcept;

    /**
     * @brief The two snapshot commands, and the unknown-command answer.
     *
     * The policy half of dump_state and load_state, written once for
     * every application: the echo lines, the refusal while recording
     * or replaying, the history a load replaces, and the error a load
     * answers with.
     * What the state *is* stays behind ISnapshotStore.
     *
     * **dump_state runs everywhere, load_state only live.**
     * A dump is a write-only projection of state a replay reproduces,
     * so a replayed run re-executes it and rewrites the same file.
     * A load reads a file whose contents no recording carries, so a
     * recorded or replayed run answers it with a deterministic
     * refusal line instead -- see wiki/libraries/console.md for the
     * whole argument.
     */
    class SnapshotCommands final : public IConsoleCommands
    {
    public:
        /**
         * @brief Construct the commands over one application's store.
         * @param store Takes and applies the application's state.
         * Must outlive this object.
         * @param dumpPath Where dump_state writes and load_state
         * reads.
         * @param loadEnabled Whether load_state may run at all; false
         * under --record and --replay -- see consoleLoadPermitted().
         */
        SnapshotCommands(
            ISnapshotStore &store,
            std::string dumpPath,
            bool loadEnabled);

        SnapshotCommands(const SnapshotCommands &) = delete;
        SnapshotCommands(SnapshotCommands &&) = delete;

        SnapshotCommands &operator=(const SnapshotCommands &) = delete;
        SnapshotCommands &operator=(SnapshotCommands &&) = delete;

        /**
         * @brief Execute one echoed command line.
         * @param command The trimmed, non-empty line.
         * @param console The console to answer into.
         * @throws SnapshotError If dump_state cannot write its file.
         * A full disk is the machine's truth rather than the run's,
         * so it ends the run instead of becoming a history line a
         * replay would then disagree about.
         */
        void execute(
            const std::string &command, ConsoleState &console) override;

    private:
        void dumpState(ConsoleState &console);

        void loadState(ConsoleState &console);

        ISnapshotStore &store;
        std::string dumpPath;
        bool loadEnabled;
    };

} // namespace antwika::console
