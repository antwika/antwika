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

    bool consoleLoadPermitted(
        const antwika::replay::ReplayCliOptions &options) noexcept
    {
        return consoleLoadPermitted(
            options.recordPath.has_value(),
            options.replayPath.has_value());
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

    std::vector<std::string> SnapshotCommands::names() const
    {
        return {"dump_state", "load_state"};
    } // GCOVR_EXCL_LINE

    void SnapshotCommands::dumpState(ConsoleState &console)
    {
        console.pushHistory("dumped state to " + dumpPath);

        store.dump(dumpPath, console.history());
    }

    void SnapshotCommands::loadState(ConsoleState &console)
    {
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
        catch (const SnapshotError &failed) // GCOVR_EXCL_LINE
        {
            console.pushHistory(
                std::string("could not load: ") + failed.what());
            return;
        }

        console.replaceHistory(std::move(carried));
        console.pushHistory("loaded state from " + dumpPath);
    }

}
