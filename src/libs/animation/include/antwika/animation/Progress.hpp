#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    /**
     * @brief How far along something is, as an exact fraction between
     * zero and one inclusive.
     *
     * This is a rational rather than a float on purpose.
     * A replay reproduces a picture by recomputing it from the tick
     * number, and two builds only agree on that if every step of the
     * arithmetic is exact.
     * Integer division is exact on every toolchain this project builds
     * with; a float divide is not something the language pins down the
     * same way, and once a fraction has been rounded once, nothing
     * downstream can tell how far it drifted.
     * Keeping the numerator and the denominator apart also means the
     * caller decides where the rounding happens, in its own units, at
     * the one point it can see what a half-pixel costs.
     *
     * Equality is on the pair, not on the value: 1/2 and 2/4 are
     * different Progress values, because a clip that ran a frame for
     * four ticks did not run it for two.
     */
    class Progress final
    {
    public:
        /**
         * @brief Build a progress of zero, as 0/1.
         */
        Progress() noexcept;

        /**
         * @brief Build a progress from an exact fraction.
         * @param numerator How much of the span has gone.
         * @param denominator How long the whole span is.
         * @throws AnimationError If the denominator is zero, or the
         * numerator is larger than the denominator.
         */
        Progress(time::Tick numerator, time::Tick denominator);

        /**
         * @brief Get how much of the span has gone.
         * @return The numerator.
         */
        [[nodiscard]] time::Tick numerator() const noexcept;

        /**
         * @brief Get how long the whole span is.
         * @return The denominator, never zero.
         */
        [[nodiscard]] time::Tick denominator() const noexcept;

        /**
         * @brief Compare two progress values field by field.
         * @param other The progress to compare against.
         * @return Whether both the numerator and the denominator match.
         */
        [[nodiscard]] bool operator==(
            const Progress &other) const noexcept = default;

    private:
        time::Tick num;
        time::Tick den;
    };

    /**
     * @brief Get a whole-number point a fraction of the way between two
     * others.
     *
     * The units are the caller's own -- pixels, cells, whatever the two
     * ends were expressed in -- since this library has no idea what is
     * being animated.
     * The division truncates towards zero, and it is the last operation,
     * so the result is the same on every toolchain rather than the same
     * to within a rounding mode.
     *
     * @param from Where the span starts.
     * @param to Where the span ends.
     * @param progress How far along the span to look.
     * @return The point that far along, exactly `from` at 0/n and
     * exactly `to` at n/n.
     * @note The span times the numerator must fit in a signed 64-bit
     * integer.  It is the numerator that carries the magnitude, not
     * the span: an eased progress arrives scaled to a far larger
     * denominator, so whatever produced the fraction is what keeps
     * this product in range.
     */
    [[nodiscard]] std::int64_t interpolate(
        std::int64_t from, std::int64_t to, Progress progress) noexcept;

} // namespace antwika::animation
