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

    /**
     * @brief Everything a spectator may see about a table, as a value.
     *
     * A value rather than a reference to the Table on purpose. Rendering
     * has to be a write-only projection of the game, and a scene handed
     * one of these structurally cannot reach the table, ask it anything,
     * or change it -- so no amount of drawing code can affect what a
     * replay reproduces. It also means a test can draw an interesting
     * table without dealing its way into one.
     */
    struct TableSnapshot
    {
        /**
         * @brief The name the table is announced under.
         */
        std::string tableName{};

        /**
         * @brief Every seat the table has, in clockwise order.
         */
        std::vector<SeatSnapshot> seats{};

        /**
         * @brief The community cards as dealt so far.
         */
        std::vector<Card> board{};

        /**
         * @brief Chips in the middle, zero once a hand is paid out.
         */
        Chips pot{};

        /**
         * @brief The forced bets in force.
         */
        Blinds blinds{};

        /**
         * @brief How far the current or last hand progressed.
         */
        Stage stage{};

        /**
         * @brief How many hands have been dealt at this table.
         */
        std::uint64_t handsPlayed = 0;

        /**
         * @brief Whether a hand is being played right now.
         */
        bool handInProgress = false;

        bool operator==(const TableSnapshot &other) const = default;
    };

    /**
     * @brief Take a snapshot of a table and who is sitting at it.
     * @param table Read for the seats, the board, the pot and the stage.
     * @param game Read for the name occupying each seat.
     * @param tableName The name the table is announced under.
     * @return What a spectator would see right now.
     */
    [[nodiscard]] TableSnapshot snapshotOf(
        const Table &table, const CashGame &game, std::string tableName);

} // namespace antwika::poker
