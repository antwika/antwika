#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/TableMemory.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/poker/PrinterMemory.hpp"

namespace antwika::poker
{

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-poker-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    struct RoomDump final
    {
        std::uint64_t bits = 0;

        antwika::holdem::DeckMemory deck;

        antwika::holdem::TableMemory table;

        std::map<std::string, antwika::holdem::Chips> balances;

        std::vector<std::string> names;

        PrinterMemory printer;

        [[nodiscard]] bool operator==(
            const RoomDump &other) const = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json roomDumpToJson(const RoomDump &dump);

    [[nodiscard]] RoomDump roomDumpFromJson(const nlohmann::json &state);

}
