#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

#include <antwika/config/Format.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/console/IJsonSnapshotStore.hpp"

namespace antwika::console::fakes
{

    enum class Refusal : std::uint8_t
    {
        None,
        Own,
        Deeper
    };

    template <typename ErrorT, typename DeeperErrorT>
    class FakeSnapshotStore final : public IJsonSnapshotStore<ErrorT>
    {
    public:
        explicit FakeSnapshotStore(
            antwika::replay::MigrationChain (*migrations)())
            : IJsonSnapshotStore<ErrorT>(
                  {.magic = "antwika-test-state-dump", .version = 1},
                  "antwika test state dump document",
                  migrations)
        {
        }

        nlohmann::json state = {{"cells", "101"}};
        std::string takenFor;
        std::string appliedFor;
        Refusal refuses = Refusal::None;

    private:
        void refuse(const std::string &what) const
        {
            if (refuses == Refusal::Own)
            {
                throw ErrorT(what);
            }

            if (refuses == Refusal::Deeper)
            {
                throw DeeperErrorT(what);
            }
        }

        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override
        {
            takenFor = path;
            refuse("nothing to take");

            return state;
        }

        void applyState(
            const std::string &path,
            const nlohmann::json &taken) override
        {
            appliedFor = path;
            refuse("a state no session could be in");

            state = taken;
        }
    };

}
