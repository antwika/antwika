#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Stage.hpp>

namespace antwika::poker
{

    using antwika::holdem::Chips;
    using antwika::holdem::SeatId;
    using antwika::holdem::Stage;

    struct PrinterNote final
    {
        Chips roundStake{};

        Stage foldedOn{};

        bool dealtIn = false;

        bool folded = false;

        [[nodiscard]] bool operator==(
            const PrinterNote &other) const = default;
    };

    struct PrinterMemory final
    {
        std::vector<PrinterNote> notes;

        std::optional<SeatId> smallBlindSeat;

        std::optional<SeatId> bigBlindSeat;

        Stage stage{};

        std::size_t boardShown = 0;

        [[nodiscard]] bool operator==(
            const PrinterMemory &other) const = default;
    };

}
