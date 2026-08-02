#pragma once

#include <compare>
#include <cstdint>

namespace antwika::pattern
{

    /**
     * @brief A position or a length in cycles, as an exact fraction.
     *
     * **The one decision the rest of this library depends on.**
     * Musical time here is a rational number of cycles, as it is in
     * Tidal and Strudel, rather than a count of subdivisions at some
     * fixed resolution.
     * A resolution -- 960 pulses to the quarter, say -- divides cleanly
     * by two, three, four, five and six, and not by seven, so it
     * quantises a septuplet and every deeply nested subdivision.
     * An exact fraction quantises nothing, and composing four time
     * transformations still lands on an exact position.
     *
     * The cost is that a denominator grows under composition and
     * eventually will not fit, which is a PatternError rather than a
     * rounding.
     *
     * **Always reduced, and compared by value**, which is deliberately
     * the opposite of antwika::animation::Progress.
     * That type compares on the pair, because a clip run for four ticks
     * did not run for two.
     * This one is a position: one half and two quarters are the same
     * moment, and a combinator that reached the same moment by a
     * different route has to agree that it did.
     */
    class Cycle final
    {
    public:
        /**
         * @brief Build a position of zero, as 0/1.
         */
        Cycle() noexcept;

        /**
         * @brief Build a whole number of cycles.
         * @param whole How many cycles.
         */
        explicit Cycle(std::int64_t whole) noexcept;

        /**
         * @brief Build an exact fraction of a cycle.
         *
         * Reduced on the way in, and the sign is carried by the
         * numerator, so the denominator is always positive.
         *
         * @param numerator The fraction's top.
         * @param denominator The fraction's bottom.
         * @throws PatternError If the denominator is zero, or if
         * normalising the sign would leave the range of the integers
         * backing it.
         */
        Cycle(std::int64_t numerator, std::int64_t denominator);

        /**
         * @brief Get the fraction's top.
         * @return The numerator, carrying the sign.
         */
        [[nodiscard]] std::int64_t numerator() const noexcept;

        /**
         * @brief Get the fraction's bottom.
         * @return The denominator, always positive.
         */
        [[nodiscard]] std::int64_t denominator() const noexcept;

        /**
         * @brief Get which cycle this position falls in.
         *
         * Floored rather than truncated, so a position before zero
         * belongs to the cycle it is inside rather than the one after.
         *
         * @return The cycle's number.
         */
        [[nodiscard]] std::int64_t floorCycle() const noexcept;

        /**
         * @brief Get where the cycle holding this position starts.
         *
         * Strudel calls this the sam, and combinators that repeat
         * something once per cycle are written in terms of it.
         *
         * @return The start, which is this position when it is already
         * a whole number of cycles.
         */
        [[nodiscard]] Cycle sam() const;

        /**
         * @brief Get where the next cycle starts.
         * @return One cycle after sam().
         */
        [[nodiscard]] Cycle nextSam() const;

        /**
         * @brief Add two positions.
         * @param other What to add.
         * @return The sum, reduced.
         * @throws PatternError If the exact result will not fit.
         */
        [[nodiscard]] Cycle operator+(const Cycle &other) const;

        /**
         * @brief Subtract one position from another.
         * @param other What to take away.
         * @return The difference, reduced.
         * @throws PatternError If the exact result will not fit.
         */
        [[nodiscard]] Cycle operator-(const Cycle &other) const;

        /**
         * @brief Multiply two fractions.
         * @param other What to multiply by.
         * @return The product, reduced.
         * @throws PatternError If the exact result will not fit.
         */
        [[nodiscard]] Cycle operator*(const Cycle &other) const;

        /**
         * @brief Divide one fraction by another.
         * @param other What to divide by.
         * @return The quotient, reduced.
         * @throws PatternError If the divisor is zero, or if the exact
         * result will not fit.
         */
        [[nodiscard]] Cycle operator/(const Cycle &other) const;

        /**
         * @brief Compare two positions by value.
         * @param other The position to compare against.
         * @return True when they are the same moment.
         */
        [[nodiscard]] bool operator==(const Cycle &other) const noexcept;

        /**
         * @brief Order two positions by value.
         *
         * Cross-multiplied in a wider integer, so ordering never throws
         * however large the denominators have grown.
         *
         * @param other The position to compare against.
         * @return How this one orders against it.
         */
        [[nodiscard]] std::strong_ordering operator<=>(
            const Cycle &other) const noexcept;

    private:
        std::int64_t num;
        std::int64_t den;
    };

} // namespace antwika::pattern
