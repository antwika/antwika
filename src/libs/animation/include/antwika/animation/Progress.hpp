#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    class Progress final
    {
    public:
        Progress() noexcept;

        Progress(time::Tick numerator, time::Tick denominator);

        [[nodiscard]] time::Tick numerator() const noexcept;

        [[nodiscard]] time::Tick denominator() const noexcept;

        [[nodiscard]] bool operator==(
            const Progress &other) const noexcept = default;

    private:
        time::Tick num;
        time::Tick den;
    };

    [[nodiscard]] std::int64_t interpolate(
        std::int64_t from, std::int64_t to, Progress progress) noexcept;

}
