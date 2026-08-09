#pragma once

#include <string>
#include <vector>

#include <antwika/replay/ReplayCli.hpp>

#include "antwika/console/IConsoleCommands.hpp"
#include "antwika/console/ISnapshotStore.hpp"

namespace antwika::console
{

    [[nodiscard]] bool consoleLoadPermitted(
        bool recording, bool replaying) noexcept;

    [[nodiscard]] bool consoleLoadPermitted(
        const antwika::replay::ReplayCliOptions &options) noexcept;

    class SnapshotCommands final : public IConsoleCommands
    {
    public:
        SnapshotCommands(
            ISnapshotStore &store,
            std::string dumpPath,
            bool loadEnabled);

        SnapshotCommands(const SnapshotCommands &) = delete;
        SnapshotCommands(SnapshotCommands &&) = delete;

        SnapshotCommands &operator=(const SnapshotCommands &) = delete;
        SnapshotCommands &operator=(SnapshotCommands &&) = delete;

        void execute(
            const std::string &command, ConsoleState &console) override;

        [[nodiscard]] std::vector<std::string> names() const override;

    private:
        void dumpState(ConsoleState &console);

        void loadState(ConsoleState &console);

        ISnapshotStore &store;
        std::string dumpPath;
        bool loadEnabled;
    };

}
