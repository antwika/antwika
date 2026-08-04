#include "antwika/console/SnapshotCommands.hpp"

#include <utility>
#include <vector>

#include "antwika/console/SnapshotError.hpp"

namespace antwika::console
{

    bool consoleLoadPermitted(bool recording, bool replaying) noexcept
    {
        return !recording && !replaying;
    }

    SnapshotCommands::SnapshotCommands(
        ISnapshotStore &store, std::string dumpPath, bool loadEnabled)
        : store(store),
          dumpPath(std::move(dumpPath)),
          loadEnabled(loadEnabled)
    {
    }

    void SnapshotCommands::execute(
        const std::string &command, ConsoleState &console)
    {
        if (command == "dump_state")
        {
            dumpState(console);
        }
        else if (command == "load_state")
        {
            loadState(console);
        }
        else
        {
            console.pushHistory("unknown command: " + command);
        }
    }

    void SnapshotCommands::dumpState(ConsoleState &console)
    {
        // Answered before the state is taken, deliberately.
        // The dump then carries the whole exchange that made it.
        console.pushHistory("dumped state to " + dumpPath);

        store.dump(dumpPath, console.history());
    }

    void SnapshotCommands::loadState(ConsoleState &console)
    {
        // The console-level twin of the game's rule on --load.
        // A load reads a file no recording carries.
        // So a recorded or replayed run refuses it outright.
        // The refusal is a history line, and so deterministic.
        // A replay typing load_state reads what the live run read.
        if (!loadEnabled)
        {
            console.pushHistory(
                "load_state: not available while recording or "
                "replaying");
            return;
        }

        std::vector<std::string> carried;

        try
        {
            carried = store.load(dumpPath);
        }
        // The excluded line's second branch is the catch's own.
        // It is taken by an exception this catch does not match.
        // The store's contract is that nothing else leaves load().
        catch (const SnapshotError &failed) // GCOVR_EXCL_LINE
        {
            console.pushHistory(
                std::string("could not load: ") + failed.what());
            return;
        }

        console.replaceHistory(std::move(carried));
        console.pushHistory("loaded state from " + dumpPath);
    }

} // namespace antwika::console
