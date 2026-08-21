#include "antwika/animation/Progress.hpp"

#include <cstdint>
#include <string>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/AnimationError.hpp"

namespace antwika::animation
{

    Progress::Progress() noexcept : numTick(0), denTick(1)
    {
    }

    Progress::Progress(time::Tick numeratorTick, time::Tick denominatorTick)
        : numTick(numeratorTick), denTick(denominatorTick)
    {
        if (denominatorTick == 0)
        {
            throw AnimationError("Progress denominator must not be zero");
        }

        if (numeratorTick > denominatorTick)
        {
            throw AnimationError(
                "Progress numerator " + std::to_string(numeratorTick)
                + " exceeds denominator " + std::to_string(denominatorTick));
        }
    }

    time::Tick Progress::numerator() const noexcept
    {
        return numTick;
    }

    time::Tick Progress::denominator() const noexcept
    {
        return denTick;
    }

    std::int64_t interpolate(
        std::int64_t fromValue, std::int64_t toValue,
        Progress progress) noexcept
    {
        const std::int64_t span = toValue - fromValue;
        const auto numerator = static_cast<std::int64_t>(progress.numerator());
        const auto denominator =
            static_cast<std::int64_t>(progress.denominator());

        return fromValue + span * numerator / denominator;
    }

}
