#include "antwika/app/TickDebt.hpp"

namespace antwika::app
{

    TickDebt::TickDebt(const time::IClock &clock) noexcept
        : clock(&clock), lastFrameAt(clock.getCurrentTime())
    {
    }

    void TickDebt::start()
    {
        lastFrameAt = clock->getCurrentTime();
        owedTime = std::chrono::nanoseconds{};
    }

    std::chrono::nanoseconds TickDebt::advance()
    {
        const auto startedAt = clock->getCurrentTime();
        const auto since =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                startedAt - lastFrameAt);

        lastFrameAt = startedAt;
        owedTime += since;

        return since;
    }

    bool TickDebt::owesTick() const noexcept
    {
        return owedTime >= kTickPeriod;
    }

    void TickDebt::payTick() noexcept
    {
        owedTime -= kTickPeriod;
    }

    void TickDebt::forgive() noexcept
    {
        owedTime = std::chrono::nanoseconds{};
    }

    std::chrono::nanoseconds TickDebt::getOwedTime() const noexcept
    {
        return owedTime;
    }

    std::chrono::time_point<std::chrono::system_clock>
    TickDebt::startedAt() const noexcept
    {
        return lastFrameAt;
    }

}
