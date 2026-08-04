#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandResult.hpp"
#include "antwika/holdem/Seat.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/Stage.hpp"

namespace antwika::holdem
{

    /**
     * @brief A deck's exact position: its order, and the deal cursor.
     *
     * The order is not derivable from a generator's state alone -- the
     * shuffle that produced it has already drawn -- so a resumed
     * session carries both.
     */
    struct DeckMemory
    {
        /** @brief Every card, in the deck's current order. */
        std::array<Card, kCardCount> cards{};

        /** @brief How many have been dealt off the top. */
        std::size_t dealt = 0;

        /**
         * @brief Compare two memories.
         * @param other The memory to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const DeckMemory &other) const = default;
    };

    /**
     * @brief A betting round's live bet and its smallest full raise.
     */
    struct BettingMemory
    {
        /** @brief What it costs to stay in. */
        Chips currentBet = 0;

        /** @brief The size of the last full raise. */
        Chips lastRaiseSize = 0;

        /**
         * @brief Compare two memories.
         * @param other The memory to compare against.
         * @return True when both fields match.
         */
        [[nodiscard]] bool operator==(
            const BettingMemory &other) const = default;
    };

    /**
     * @brief One table's whole standing, as a value.
     *
     * Everything Table::restore() needs to stand a fresh table at the
     * exact instant remember() was called -- mid-hand included, since
     * the seats, the board, the stage, the live bet and whose turn it
     * is are all here.
     * The deck is deliberately a separate memory: it belongs to
     * whoever owns the shuffle, exactly as the generator's state does.
     */
    struct TableMemory
    {
        /** @brief Every seat, in seat order. */
        std::vector<Seat> seats;

        /** @brief The last finished hand's outcome, if any. */
        std::optional<HandResult> result;

        /** @brief Whose turn it is, while a hand is in progress. */
        std::optional<SeatId> toAct;

        /** @brief Chips in the middle. */
        Chips pot = 0;

        /** @brief The live bet on the current street. */
        BettingMemory betting;

        /** @brief How far the current or last hand progressed. */
        Stage stage = Stage::PreFlop;

        /** @brief The community cards turned so far. */
        std::vector<Card> board;

        /** @brief How many hands this table has dealt. */
        std::uint64_t handCount = 0;

        /** @brief The dealer button's seat. */
        SeatId button{};

        /** @brief Whether a hand is being played. */
        bool handInProgress = false;

        /**
         * @brief Compare two memories.
         * @param other The memory to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const TableMemory &other) const = default;
    };

} // namespace antwika::holdem
