#pragma once

#include <compare>
#include <cstdint>

namespace antwika::pattern
{

    class Cycle final
    {
    public:
        Cycle() noexcept;

        explicit Cycle(std::int64_t whole) noexcept;

        Cycle(std::int64_t numerator, std::int64_t denominator);

        [[nodiscard]] std::int64_t numerator() const noexcept;

        [[nodiscard]] std::int64_t denominator() const noexcept;

        [[nodiscard]] std::int64_t floorCycle() const noexcept;

        [[nodiscard]] Cycle sam() const;

        [[nodiscard]] Cycle nextSam() const;

        [[nodiscard]] Cycle operator+(const Cycle &other) const;

        [[nodiscard]] Cycle operator-(const Cycle &other) const;

        [[nodiscard]] Cycle operator*(const Cycle &other) const;

        [[nodiscard]] Cycle operator/(const Cycle &other) const;

        [[nodiscard]] bool operator==(const Cycle &other) const noexcept;

        [[nodiscard]] std::strong_ordering operator<=>(
            const Cycle &other) const noexcept;

    private:
        std::int64_t num;
        std::int64_t den;
    };

}
