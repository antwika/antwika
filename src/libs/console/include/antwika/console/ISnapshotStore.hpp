#pragma once

#include <string>
#include <vector>

namespace antwika::console
{

    class ISnapshotStore
    {
    public:
        virtual ~ISnapshotStore() = default;

        virtual void dump(
            const std::string &path,
            const std::vector<std::string> &console) = 0;

        [[nodiscard]] virtual std::vector<std::string> load(
            const std::string &path) = 0;
    };

}
