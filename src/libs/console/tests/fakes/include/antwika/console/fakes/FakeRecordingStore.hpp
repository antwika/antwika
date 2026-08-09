#pragma once

#include <string>
#include <vector>

#include "antwika/console/ISnapshotStore.hpp"

namespace antwika::console::fakes
{

    struct FakeRecordingStore final : ISnapshotStore
    {
        std::string dumpedTo;

        void dump(
            const std::string &path,
            const std::vector<std::string> &) override
        {
            dumpedTo = path;
        }

        [[nodiscard]] std::vector<std::string> load(
            const std::string &) override
        {
            return {};
        }
    };

}
