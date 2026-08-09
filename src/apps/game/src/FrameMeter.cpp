#include "antwika/game/FrameMeter.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr std::uint64_t kMillisPerSecond = 1000;
    }

    FrameMeter::FrameMeter(const IClock &clock) noexcept : clock(clock)
    {
    }

    void FrameMeter::record()
    {
        const auto now = clock.now();

        if (!windowStart.has_value())
        {
            windowStart = now;
            return;
        }

        ++counted;

        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                now - *windowStart);

        if (elapsed < kFpsWindow)
        {
            return;
        }

        rate = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(counted) * kMillisPerSecond
            / static_cast<std::uint64_t>(elapsed.count()));

        counted = 0;
        windowStart = now;
    }

    std::optional<std::uint32_t> FrameMeter::perSecond() const noexcept
    {
        return rate;
    }

}
