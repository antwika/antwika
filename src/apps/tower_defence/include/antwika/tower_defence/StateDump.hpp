#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <stdexcept>
#include <string_view>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/tower_defence/Campaign.hpp"

namespace antwika::tower_defence
{

    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-tower-defence-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    struct StateDump final
    {
        CampaignMemory campaign;

        std::uint64_t bestScore = 0;

        [[nodiscard]] bool operator==(const StateDump &) const
            = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

}
