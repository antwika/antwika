#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::animation
{

    class Progress final
    {
    public:
        Progress() noexcept;

        Progress(time::Tick numeratorTick, time::Tick denominatorTick);

        [[nodiscard]] time::Tick numerator() const noexcept;

        [[nodiscard]] time::Tick denominator() const noexcept;

        [[nodiscard]] bool operator==(
            const Progress &other) const noexcept = default;

    private:
        time::Tick numTick;
        time::Tick denTick;
    };

    [[nodiscard]] std::int64_t interpolate(
        std::int64_t fromValue, std::int64_t toValue,
        Progress progress) noexcept;

}
