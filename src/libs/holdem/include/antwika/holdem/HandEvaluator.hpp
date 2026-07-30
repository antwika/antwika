#pragma once

#include <cstddef>
#include <span>

#include "antwika/holdem/Card.hpp"
#include "antwika/holdem/HandValue.hpp"

namespace antwika::holdem
{

    /**
     * @brief Fewest cards evaluate() accepts: a complete made hand.
     */
    inline constexpr std::size_t kMinEvaluatedCards = 5;

    /**
     * @brief Most cards evaluate() accepts: two hole cards and a full
     * board, of which it picks the best five.
     */
    inline constexpr std::size_t kMaxEvaluatedCards = 7;

    /**
     * @brief Score a set of cards as the single comparable number
     * describing the best five-card hand it contains.
     *
     * The cards need no ordering and the five that end up counting are
     * never materialised: the whole evaluation runs on 13-bit rank
     * masks, one per suit, folded together with shifts, ands and ors.
     * Duplicate ranks fall out of pairwise intersections of those four
     * masks, straights out of `m & m>>1 & m>>2 & m>>3 & m>>4` over a
     * mask extended by one bit so the ace can also play low, and
     * flushes out of a population count. Nothing is enumerated, so
     * evaluating seven cards costs the same as evaluating five.
     *
     * @param cards Between kMinEvaluatedCards and kMaxEvaluatedCards
     * distinct cards.
     * @return The strength of the best five-card hand among them.
     * @throws HandEvaluationError If the card count is out of range, a
     * card appears twice, or a card's packed value is not a real card.
     */
    [[nodiscard]] HandValue evaluate(std::span<const Card> cards);

} // namespace antwika::holdem
