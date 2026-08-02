#pragma once

#include <compare>
#include <cstdint>

namespace antwika::pattern
{

    /** @brief How many bits of a ParamValue lie below the point. */
    inline constexpr int kFractionBits = 32;

    /**
     * @brief What one control carries, in fixed point.
     *
     * **Fixed point rather than an exact rational**, which is the one
     * place this library stops being rational and does so on purpose.
     * Time is composed by multiplication and has to stay exact, so a
     * Cycle is a fraction and its denominator is allowed to grow.
     * A parameter is *modulated* instead -- added to, offset, scaled by
     * a depth, over and over -- and a fraction under that treatment
     * grows a denominator without bound until it will not fit.
     * A fixed-point value has one representation whatever is done to it.
     *
     * Thirty-two bits below the point resolves about a
     * four-thousand-millionth, which is finer than any parameter a
     * person writes down, and leaves whole numbers up to two thousand
     * million.
     *
     * There is deliberately no multiplication of two values yet.
     * Nothing needs one, and doing it exactly needs an integer twice as
     * wide -- which this library has already decided not to reach for,
     * in Cycle's comparison.
     */
    class ParamValue final
    {
    public:
        /**
         * @brief Build a value of zero.
         */
        ParamValue() noexcept;

        /**
         * @brief Build a whole number.
         * @param whole The number.
         * @throws PatternError If it will not fit above the point.
         */
        explicit ParamValue(std::int64_t whole);

        /**
         * @brief Build an exact fraction, rounded to what fits below the
         * point.
         * @param numerator The fraction's top.
         * @param denominator The fraction's bottom.
         * @throws PatternError If the denominator is zero, or the result
         * will not fit.
         */
        ParamValue(std::int64_t numerator, std::int64_t denominator);

        /**
         * @brief Build a value from its stored bits.
         * @param raw The bits, as returned by raw().
         * @return The value those bits stand for.
         */
        [[nodiscard]] static ParamValue fromRaw(std::int64_t raw) noexcept;

        /**
         * @brief Get the stored bits.
         * @return The value scaled up by two to the kFractionBits.
         */
        [[nodiscard]] std::int64_t raw() const noexcept;

        /**
         * @brief Get this as a plain number.
         *
         * For the projection side only -- a frequency handed to an
         * oscillator, a gain handed to a mixer.
         * Nothing that a replay reproduces may read this, since the
         * whole reason the stored form is fixed point is that it is
         * exact and this is not.
         *
         * @return The value, approximately.
         */
        [[nodiscard]] double approximate() const noexcept;

        /**
         * @brief Add two values.
         * @param other What to add.
         * @return The sum.
         * @throws PatternError If the sum will not fit.
         */
        [[nodiscard]] ParamValue operator+(const ParamValue &other) const;

        /**
         * @brief Subtract one value from another.
         * @param other What to take away.
         * @return The difference.
         * @throws PatternError If the difference will not fit.
         */
        [[nodiscard]] ParamValue operator-(const ParamValue &other) const;

        /**
         * @brief Compare two values.
         * @param other The value to compare against.
         * @return True when they stand for the same number.
         */
        [[nodiscard]] bool operator==(const ParamValue &other) const
            noexcept = default;

        /**
         * @brief Order two values.
         * @param other The value to compare against.
         * @return How this one orders against it.
         */
        [[nodiscard]] std::strong_ordering operator<=>(
            const ParamValue &other) const noexcept = default;

    private:
        std::int64_t bits;
    };

} // namespace antwika::pattern
