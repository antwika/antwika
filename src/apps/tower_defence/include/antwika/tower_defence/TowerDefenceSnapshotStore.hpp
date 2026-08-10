#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <string>

#include <antwika/console/IJsonSnapshotStore.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/StateDump.hpp"

namespace antwika::tower_defence
{

    class TowerDefenceSnapshotStore final
        : public antwika::console::IJsonSnapshotStore<StateDumpError>
    {
    public:
        TowerDefenceSnapshotStore(
            Campaign &campaign, std::uint64_t &best) noexcept;

    private:
        [[nodiscard]] nlohmann::json takeState(
            const std::string &path) override;

        void applyState(
            const std::string &path,
            const nlohmann::json &state) override;

        Campaign &campaign;
        std::uint64_t &best;
    };

}
