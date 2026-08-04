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

    /**
     * @brief What one seat did in the hand being written up.
     */
    struct PrinterNote
    {
        /** @brief Chips this seat put in on the current street. */
        Chips roundStake{};

        /** @brief The street this seat folded on, if it folded. */
        Stage foldedOn{};

        /** @brief Whether this seat was dealt into the hand. */
        bool dealtIn = false;

        /** @brief Whether this seat has folded. */
        bool folded = false;

        /**
         * @brief Compare two notes.
         * @param other The note to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const PrinterNote &other) const = default;
    };

    /**
     * @brief The hand history's own mid-hand standing, as a value.
     *
     * Narration rather than rules, but carried across the ticks of a
     * hand -- a dump that skipped it would resume a hand history with
     * the wrong header, so the printer remembers and restores exactly
     * as the table does.
     */
    struct PrinterMemory
    {
        /** @brief One note per seat, in seat order. */
        std::vector<PrinterNote> notes;

        /** @brief Who posted the small blind this hand, if anyone. */
        std::optional<SeatId> smallBlindSeat;

        /** @brief Who posted the big blind this hand, if anyone. */
        std::optional<SeatId> bigBlindSeat;

        /** @brief The street the narration has reached. */
        Stage stage{};

        /** @brief How many board cards have been written out. */
        std::size_t boardShown = 0;

        /**
         * @brief Compare two memories.
         * @param other The memory to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const PrinterMemory &other) const = default;
    };

} // namespace antwika::poker
