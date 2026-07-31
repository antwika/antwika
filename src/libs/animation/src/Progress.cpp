#include "antwika/animation/Progress.hpp"

#include <cstdint>
#include <string>

#include <antwika/time/Tick.hpp>

#include "antwika/animation/AnimationError.hpp"

namespace antwika::animation
{

    Progress::Progress() noexcept : num(0), den(1)
    {
    }

    Progress::Progress(time::Tick numerator, time::Tick denominator)
        : num(numerator), den(denominator)
    {
        if (denominator == 0)
        {
            throw AnimationError("Progress denominator must not be zero");
        }

        if (numerator > denominator)
        {
            throw AnimationError(
                "Progress numerator " + std::to_string(numerator)
                + " exceeds denominator " + std::to_string(denominator));
        }
    }

    time::Tick Progress::numerator() const noexcept
    {
        return num;
    }

    time::Tick Progress::denominator() const noexcept
    {
        return den;
    }

    std::int64_t interpolate(
        std::int64_t from, std::int64_t to, Progress progress) noexcept
    {
        const std::int64_t span = to - from;
        const auto numerator = static_cast<std::int64_t>(progress.numerator());
        const auto denominator =
            static_cast<std::int64_t>(progress.denominator());

        return from + span * numerator / denominator;
    }

} // namespace antwika::animation
