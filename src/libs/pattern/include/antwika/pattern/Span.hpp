#pragma once

#include <optional>
#include <vector>

#include "antwika/pattern/Cycle.hpp"

namespace antwika::pattern
{

    /**
     * @brief A stretch of musical time, half-open.
     *
     * **Half-open everywhere, without exception.**
     * A sequencer advances a window across a run, and two windows that
     * shared an endpoint would trigger whatever sat on it twice, while
     * two that left a gap would drop it.
     * Neither shows up in a test of one window, which is why the rule is
     * in the type rather than in each caller.
     *
     * A span that ends where it began is refused rather than treated as
     * empty, because an empty query is always a caller's mistake here:
     * every combinator either forwards its window or maps it, and one
     * that produced an empty span produced it from arithmetic that had
     * already gone wrong.
     */
    class Span final
    {
    public:
        /**
         * @brief Build a stretch between two positions.
         * @param begin Where it starts, included.
         * @param end Where it stops, excluded.
         * @throws PatternError If it ends where it began or before it.
         */
        Span(Cycle begin, Cycle end);

        /**
         * @brief Get where it starts.
         * @return The first position inside it.
         */
        [[nodiscard]] const Cycle &begin() const noexcept;

        /**
         * @brief Get where it stops.
         * @return The first position after it.
         */
        [[nodiscard]] const Cycle &end() const noexcept;

        /**
         * @brief Get how long it is.
         * @return The length, always above zero.
         * @throws PatternError If the exact length will not fit.
         */
        [[nodiscard]] Cycle length() const;

        /**
         * @brief Get the part two stretches share.
         * @param other The stretch to intersect with.
         * @return What they share, or nothing when they only touch or
         * do not meet at all.
         */
        [[nodiscard]] std::optional<Span> intersect(
            const Span &other) const;

        /**
         * @brief Split this into one piece per cycle it touches.
         *
         * What every combinator repeating something once per cycle is
         * written in terms of, since a query spanning three cycles has
         * to be answered as three separate questions.
         *
         * @return The pieces, in ascending order, together covering
         * exactly this stretch and no more.
         * @throws PatternError If the arithmetic will not fit.
         */
        [[nodiscard]] std::vector<Span> spanCycles() const;

        /**
         * @brief Compare two stretches.
         * @param other The stretch to compare against.
         * @return True when both ends match.
         */
        [[nodiscard]] bool operator==(const Span &other) const noexcept
            = default;

    private:
        Cycle from;
        Cycle to;
    };

} // namespace antwika::pattern
