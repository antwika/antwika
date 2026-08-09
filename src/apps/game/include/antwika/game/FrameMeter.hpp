#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include <antwika/time/IClock.hpp>

namespace antwika::game
{

    using antwika::time::IClock;

    inline constexpr std::chrono::milliseconds kFpsWindow{1000};

    class FrameMeter final
    {
    public:
        explicit FrameMeter(const IClock &clock) noexcept;

        FrameMeter(const FrameMeter &) = delete;
        FrameMeter(FrameMeter &&) = delete;

        FrameMeter &operator=(const FrameMeter &) = delete;
        FrameMeter &operator=(FrameMeter &&) = delete;

        void record();

        [[nodiscard]] std::optional<std::uint32_t> perSecond()
            const noexcept;

    private:
        using TimePoint =
            std::chrono::time_point<std::chrono::system_clock>;

        const IClock &clock;
        std::optional<TimePoint> windowStart{};
        std::uint32_t counted = 0;
        std::optional<std::uint32_t> rate{};
    };

}
