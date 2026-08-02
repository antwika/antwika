#include "antwika/pattern/Cycle.hpp"

#include <compare>
#include <cstdint>
#include <limits>
#include <string>

#include "antwika/pattern/PatternError.hpp"

namespace antwika::pattern
{

    namespace
    {
        // Every toolchain this project builds with is GCC or Clang.
        // The overflow builtins are available on all three.
        // Checking rather than widening keeps the stored type one word.
        [[nodiscard]] std::int64_t mulExact(
            std::int64_t left, std::int64_t right)
        {
            std::int64_t result = 0;

            if (__builtin_mul_overflow(left, right, &result))
            {
                throw PatternError(
                    "antwika::pattern: " + std::to_string(left) + " times "
                    + std::to_string(right)
                    + " leaves the range an exact cycle is held in");
            }

            return result;
        }

        [[nodiscard]] std::int64_t addExact(
            std::int64_t left, std::int64_t right)
        {
            std::int64_t result = 0;

            if (__builtin_add_overflow(left, right, &result))
            {
                throw PatternError(
                    "antwika::pattern: " + std::to_string(left) + " plus "
                    + std::to_string(right)
                    + " leaves the range an exact cycle is held in");
            }

            return result;
        }

        [[nodiscard]] std::int64_t subExact(
            std::int64_t left, std::int64_t right)
        {
            std::int64_t result = 0;

            if (__builtin_sub_overflow(left, right, &result))
            {
                throw PatternError(
                    "antwika::pattern: " + std::to_string(left) + " less "
                    + std::to_string(right)
                    + " leaves the range an exact cycle is held in");
            }

            return result;
        }

        // Two's-complement negation done in unsigned.
        // That is defined where negating a signed value is not.
        // The most negative integer is what std::gcd may not be handed.
        // A numerator is allowed to hold it.
        [[nodiscard]] std::uint64_t magnitude(std::int64_t value) noexcept
        {
            const auto bits = static_cast<std::uint64_t>(value);

            return value < 0 ? ~bits + 1U : bits;
        }

        // Euclid over magnitudes rather than std::gcd over the values.
        // The result divides a positive denominator.
        // So it always fits back into a signed integer.
        [[nodiscard]] std::int64_t commonDivisor(
            std::int64_t left, std::int64_t right) noexcept
        {
            auto larger = magnitude(left);
            auto smaller = magnitude(right);

            while (smaller != 0)
            {
                const auto remainder = larger % smaller;

                larger = smaller;
                smaller = remainder;
            }

            return static_cast<std::int64_t>(larger);
        }

        // Floored division, and the remainder it leaves.
        // The product q * bottom is at most bottom away from top.
        // It is representable whenever top is.
        // No step here can overflow.
        struct Divided
        {
            std::int64_t whole = 0;
            std::int64_t rest = 0;
        };

        [[nodiscard]] Divided divideFloored(
            std::int64_t top, std::int64_t bottom) noexcept
        {
            auto whole = top / bottom;

            if (top % bottom != 0 && top < 0)
            {
                --whole;
            }

            return Divided{.whole = whole, .rest = top - whole * bottom};
        }

        // Compares two fractions by expanding both as continued fractions.
        // That is exact and cannot overflow.
        // Cross-multiplying is the obvious way and needs a doubled integer.
        // Pedantic warnings rightly refuse the one GCC and Clang offer.
        // So this walks instead of widening.
        // Each round compares whole parts.
        // If those agree it compares the reciprocals of what is left.
        // Taking a reciprocal reverses the answer.
        [[nodiscard]] std::strong_ordering compareExactly(
            std::int64_t leftTop,
            std::int64_t leftBottom,
            std::int64_t rightTop,
            std::int64_t rightBottom) noexcept
        {
            bool reversed = false;

            for (;;)
            {
                const auto left = divideFloored(leftTop, leftBottom);
                const auto right = divideFloored(rightTop, rightBottom);

                if (left.whole != right.whole)
                {
                    // Compared as a plain less-than, not by spaceship.
                    // The two whole parts cannot be equal here.
                    // An unreachable third answer never gets covered.
                    const auto smaller = left.whole < right.whole;

                    return smaller != reversed
                        ? std::strong_ordering::less
                        : std::strong_ordering::greater;
                }

                if (left.rest == 0 && right.rest == 0)
                {
                    return std::strong_ordering::equal;
                }

                if (left.rest == 0)
                {
                    return reversed ? std::strong_ordering::greater
                                    : std::strong_ordering::less;
                }

                if (right.rest == 0)
                {
                    return reversed ? std::strong_ordering::less
                                    : std::strong_ordering::greater;
                }

                leftTop = leftBottom;
                leftBottom = left.rest;
                rightTop = rightBottom;
                rightBottom = right.rest;

                reversed = !reversed;
            }
        }
    } // namespace

    Cycle::Cycle() noexcept : num(0), den(1)
    {
    }

    Cycle::Cycle(std::int64_t whole) noexcept : num(whole), den(1)
    {
    }

    Cycle::Cycle(std::int64_t numerator, std::int64_t denominator)
        : num(numerator), den(denominator)
    {
        if (den == 0)
        {
            throw PatternError(
                "antwika::pattern: a cycle over a denominator of zero is "
                "not a position");
        }

        if (den < 0)
        {
            // The sign belongs to the numerator.
            // Ordering and reduction then have one case rather than four.
            // Negating the most negative integer is what cannot be done.
            if (num == std::numeric_limits<std::int64_t>::min()
                || den == std::numeric_limits<std::int64_t>::min())
            {
                throw PatternError(
                    "antwika::pattern: a cycle's sign cannot be moved to "
                    "its numerator without leaving the range it is held "
                    "in");
            }

            num = -num;
            den = -den;
        }

        const auto divisor = commonDivisor(num, den);

        num /= divisor;
        den /= divisor;
    }

    std::int64_t Cycle::numerator() const noexcept
    {
        return num;
    }

    std::int64_t Cycle::denominator() const noexcept
    {
        return den;
    }

    std::int64_t Cycle::floorCycle() const noexcept
    {
        const auto quotient = num / den;

        // Truncation rounds towards zero.
        // A position before zero belongs to the cycle it is inside.
        if (num % den != 0 && num < 0)
        {
            return quotient - 1;
        }

        return quotient;
    }

    Cycle Cycle::sam() const
    {
        return Cycle(floorCycle());
    }

    Cycle Cycle::nextSam() const
    {
        return Cycle(addExact(floorCycle(), 1));
    }

    Cycle Cycle::operator+(const Cycle &other) const
    {
        return Cycle(
            addExact(mulExact(num, other.den), mulExact(other.num, den)),
            mulExact(den, other.den));
    }

    Cycle Cycle::operator-(const Cycle &other) const
    {
        return Cycle(
            subExact(mulExact(num, other.den), mulExact(other.num, den)),
            mulExact(den, other.den));
    }

    Cycle Cycle::operator*(const Cycle &other) const
    {
        return Cycle(mulExact(num, other.num), mulExact(den, other.den));
    }

    Cycle Cycle::operator/(const Cycle &other) const
    {
        if (other.num == 0)
        {
            throw PatternError(
                "antwika::pattern: a cycle cannot be divided by nothing");
        }

        return Cycle(mulExact(num, other.den), mulExact(den, other.num));
    }

    bool Cycle::operator==(const Cycle &other) const noexcept
    {
        // Both are reduced with a positive denominator.
        // Equal values are therefore identical pairs.
        // No cross-multiply is needed.
        return num == other.num && den == other.den;
    }

    std::strong_ordering Cycle::operator<=>(
        const Cycle &other) const noexcept
    {
        return compareExactly(num, den, other.num, other.den);
    }

} // namespace antwika::pattern
