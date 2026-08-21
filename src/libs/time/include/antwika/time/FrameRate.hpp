#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>

namespace antwika::time
{

    inline constexpr std::size_t kFrameSampleCount = 32;

    class FrameRate final
    {
    public:
        void record(std::chrono::nanoseconds span);

        [[nodiscard]] std::optional<std::chrono::nanoseconds>
            averageFrameTime() const;

        [[nodiscard]] std::optional<float> perSecond() const;

    private:
        std::array<std::chrono::nanoseconds, kFrameSampleCount> spans{};
        std::size_t slotIndex = 0;
        std::size_t sampleCount = 0;
    };

    [[nodiscard]] std::string formatFrameRate(
        std::optional<float> perSecond);

    [[nodiscard]] std::string formatFrameTime(
        std::optional<std::chrono::nanoseconds> averageFrameTime);

}
