#pragma once

#include <string>
#include <vector>

#include "antwika/console/ISnapshotStore.hpp"
#include "antwika/console/SnapshotError.hpp"

namespace antwika::console::fakes
{

    struct FakeReplayingStore final : ISnapshotStore
    {
        std::string dumpedTo;
        std::vector<std::string> dumpedConsole;
        std::vector<std::string> answers{"> dump_state", "dumped"};
        bool refuse = false;

        void dump(
            const std::string &path,
            const std::vector<std::string> &console) override
        {
            dumpedTo = path;
            dumpedConsole = console;
        }

        [[nodiscard]] std::vector<std::string> load(
            const std::string &) override
        {
            if (refuse)
            {
                throw SnapshotError("no such dump");
            }

            return answers;
        }
    };

}
