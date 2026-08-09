#pragma once

#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <antwika/input/Position.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/CellCoordinate.hpp"

namespace antwika::life
{

    class StateDumpError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    inline constexpr std::string_view kStateDumpMagic =
        "antwika-life-state-dump";

    inline constexpr std::uint32_t kStateDumpVersion = 1;

    struct StateDump final
    {
        Board board;

        bool dragging = false;

        std::vector<CellCoordinate> visited;

        std::optional<antwika::input::Position> lastDrag = std::nullopt;

        [[nodiscard]] bool operator==(
            const StateDump &other) const = default;
    };

    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

}
