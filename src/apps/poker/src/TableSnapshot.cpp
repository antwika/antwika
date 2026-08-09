#include "antwika/poker/TableSnapshot.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include <antwika/holdem/SeatId.hpp>

#include "antwika/poker/SeatSnapshot.hpp"

namespace antwika::poker
{

    using antwika::holdem::makeSeatId;

    TableSnapshot snapshotOf(
        const Table &table, const CashGame &game, std::string tableName)
    {
        const auto toAct = table.seatToAct();
        const auto button = table.button();

        std::vector<SeatSnapshot> seats;
        seats.reserve(table.seatCount());

        for (std::size_t index = 0; index < table.seatCount(); ++index)
        {
            const auto id = makeSeatId(index);
            const auto &seat = table.seatAt(id);

            seats.push_back(SeatSnapshot{ // GCOVR_EXCL_LINE
                .name = game.playerAt(id).value_or(std::string{}),
                .stack = seat.stack,
                .committed = seat.committed,
                .roundCommitted = seat.roundCommitted,
                .holeCards = seat.holeCards,
                .occupied = seat.occupied,
                .inHand = seat.inHand,
                .isButton = id == button,
                .isToAct = toAct.has_value() && *toAct == id,
            });
        }

        return TableSnapshot{ // GCOVR_EXCL_LINE
            .tableName = std::move(tableName),
            .seats = std::move(seats),
            .board = table.board(),
            .pot = table.pot(),
            .blinds = table.blinds(),
            .stage = table.stage(),
            .handsPlayed = table.handsPlayed(),
            .handInProgress = table.isHandInProgress(),
        };
    }

}
