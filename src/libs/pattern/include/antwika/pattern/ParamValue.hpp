#pragma once

#include <compare>
#include <cstdint>

namespace antwika::pattern
{

    inline constexpr int kFractionBits = 32;

    class ParamValue final
    {
    public:
        ParamValue() noexcept;

        explicit ParamValue(std::int64_t whole);

        ParamValue(std::int64_t numerator, std::int64_t denominator);

        [[nodiscard]] static ParamValue fromRaw(std::int64_t raw) noexcept;

        [[nodiscard]] std::int64_t raw() const noexcept;

        [[nodiscard]] double approximate() const noexcept;

        [[nodiscard]] ParamValue operator+(const ParamValue &other) const;

        [[nodiscard]] ParamValue operator-(const ParamValue &other) const;

        [[nodiscard]] bool operator==(const ParamValue &other) const
            noexcept = default;

        [[nodiscard]] std::strong_ordering operator<=>(
            const ParamValue &other) const noexcept = default;

    private:
        std::int64_t bits;
    };

}
