#include "antwika/pattern/ParamValue.hpp"

#include <cstdint>
#include <limits>
#include <string>

#include "antwika/pattern/PatternError.hpp"

namespace antwika::pattern
{

    namespace
    {
        constexpr std::int64_t kOne = std::int64_t{1} << kFractionBits;

        [[nodiscard]] std::int64_t scaled(std::int64_t whole)
        {
            std::int64_t result = 0;

            if (__builtin_mul_overflow(whole, kOne, &result))
            {
                throw PatternError(
                    "antwika::pattern: " + std::to_string(whole)
                    + " leaves the range a parameter is held in");
            }

            return result;
        }
    }

    ParamValue::ParamValue() noexcept : bits(0)
    {
    }

    ParamValue::ParamValue(std::int64_t whole) : bits(scaled(whole))
    {
    }

    ParamValue::ParamValue(
        std::int64_t numerator, std::int64_t denominator)
        : bits(0)
    {
        if (denominator == 0)
        {
            throw PatternError(
                "antwika::pattern: a parameter over a denominator of "
                "zero is not a value");
        }

        if (denominator < 0)
        {
            if (numerator == std::numeric_limits<std::int64_t>::min()
                || denominator
                       == std::numeric_limits<std::int64_t>::min())
            {
                throw PatternError(
                    "antwika::pattern: a parameter's sign cannot be "
                    "moved to its numerator without leaving the range "
                    "it is held in");
            }

            numerator = -numerator;
            denominator = -denominator;
        }

        bits = scaled(numerator) / denominator;
    }

    ParamValue ParamValue::fromRaw(std::int64_t raw) noexcept
    {
        ParamValue value;
        value.bits = raw;

        return value;
    }

    std::int64_t ParamValue::raw() const noexcept
    {
        return bits;
    }

    double ParamValue::approximate() const noexcept
    {
        return static_cast<double>(bits) / static_cast<double>(kOne);
    }

    ParamValue ParamValue::operator+(const ParamValue &other) const
    {
        std::int64_t result = 0;

        if (__builtin_add_overflow(bits, other.bits, &result))
        {
            throw PatternError(
                "antwika::pattern: adding two parameters leaves the "
                "range one is held in");
        }

        return fromRaw(result);
    }

    ParamValue ParamValue::operator-(const ParamValue &other) const
    {
        std::int64_t result = 0;

        if (__builtin_sub_overflow(bits, other.bits, &result))
        {
            throw PatternError(
                "antwika::pattern: subtracting two parameters leaves the "
                "range one is held in");
        }

        return fromRaw(result);
    }

}
