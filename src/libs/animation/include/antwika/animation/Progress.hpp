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

        [[nodiscard]] time::Tick getNumerator() const noexcept;

        [[nodiscard]] time::Tick getDenominator() const noexcept;

        [[nodiscard]] bool operator==(
            const Progress &other) const noexcept = default;

    private:
        time::Tick numTick;
        time::Tick denTick;
    };

    [[nodiscard]] std::int64_t getInterpolate(
        std::int64_t fromValue, std::int64_t toValue,
        Progress progress) noexcept;

}
