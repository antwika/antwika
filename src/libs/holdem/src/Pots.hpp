#pragma once

#include <span>
#include <vector>

#include "antwika/holdem/Chips.hpp"
#include "antwika/holdem/HandValue.hpp"
#include "antwika/holdem/Payout.hpp"
#include "antwika/holdem/SeatId.hpp"
#include "antwika/holdem/SidePot.hpp"

/**
 * @file
 * @brief Splitting a pot into side pots and paying them out.
 *
 * Private to the library: the two steps are separated so each can be
 * checked on its own, but a caller only ever wants both, and only ever
 * with inputs Table has already made consistent.
 */
namespace antwika::holdem
{

    /**
     * @brief Carve a hand's total commitments into winnable layers.
     *
     * A layer is created at each distinct amount an eligible seat put
     * in, and holds every seat's contribution up to that amount. Chips
     * committed above the highest eligible amount -- a folded player's
     * over-bet -- fall into the top layer, since no lower layer could
     * absorb them.
     *
     * @param committed What each seat put in over the whole hand,
     * indexed by seat.
     * @param eligibleSeats The seats still eligible to win, in ascending
     * order; every other seat's chips stay in play but can only be won
     * by somebody else.
     * @return The layers, smallest first. Empty if no eligible seat put
     * anything in.
     */
    [[nodiscard]] std::vector<SidePot> buildSidePots(
        std::span<const Chips> committed,
        std::span<const SeatId> eligibleSeats);

    /**
     * @brief Award each layer to the strongest hand among its
     * contenders.
     *
     * A layer that cannot be divided evenly between tied winners leaves
     * an odd chip or two over; those go to the winners nearest the left
     * of the button, which is what payoutOrder expresses.
     *
     * Precondition: every pot has at least one contender, and every
     * contender indexes both valuesBySeat and payoutOrder.
     *
     * @param pots The layers to award.
     * @param valuesBySeat Each seat's hand strength, indexed by seat.
     * @param payoutOrder Every seat, starting from the one left of the
     * button, which fixes who receives an odd chip first.
     * @return What each paid seat is owed, in ascending seat order.
     * Seats owed nothing are left out.
     */
    [[nodiscard]] std::vector<Payout> distributePots(
        std::span<const SidePot> pots,
        std::span<const HandValue> valuesBySeat,
        std::span<const SeatId> payoutOrder);

} // namespace antwika::holdem
