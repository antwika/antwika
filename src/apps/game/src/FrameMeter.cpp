#include "antwika/game/FrameMeter.hpp"

namespace antwika::game
{

    namespace
    {
        // What turns a count over some milliseconds into a rate.
        // Not kFpsWindow, which is the same number by coincidence.
        // It would stop being one the day the window changed.
        constexpr std::uint64_t kMillisPerSecond = 1000;
    } // namespace

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

        // Sixty-four bits, so a very fast machine cannot wrap the count.
        // The divisor cannot be zero.
        // The window is positive and has just elapsed.
        rate = static_cast<std::uint32_t>(
            static_cast<std::uint64_t>(counted) * kMillisPerSecond
            / static_cast<std::uint64_t>(elapsed.count()));

        counted = 0;
        windowStart = now;
    }

    std::uint32_t FrameMeter::perSecond() const noexcept
    {
        return rate;
    }

} // namespace antwika::game
