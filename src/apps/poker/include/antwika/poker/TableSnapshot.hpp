#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <antwika/holdem/Blinds.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/Stage.hpp>
#include <antwika/holdem/Table.hpp>

#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/SeatSnapshot.hpp"

namespace antwika::poker
{

    using antwika::holdem::Blinds;
    using antwika::holdem::Stage;
    using antwika::holdem::Table;

    struct TableSnapshot final
    {
        std::string tableName{};

        std::vector<SeatSnapshot> seats{};

        std::vector<Card> board{};

        Chips pot{};

        Blinds blinds{};

        Stage stage{};

        std::uint64_t handsPlayed = 0;

        bool handInProgress = false;

        bool operator==(const TableSnapshot &other) const = default;
    };

    [[nodiscard]] TableSnapshot snapshotOf(
        const Table &table, const CashGame &game, std::string tableName);

}
