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
            getAverageFrameTime() const;

        [[nodiscard]] std::optional<float> getPerSecond() const;

    private:
        std::array<std::chrono::nanoseconds, kFrameSampleCount> spans{};
        std::size_t slotIndex = 0;
        std::size_t sampleCount = 0;
    };

    [[nodiscard]] std::string getFormatFrameRate(
        std::optional<float> perSecond);

    [[nodiscard]] std::string getFormatFrameTime(
        std::optional<std::chrono::nanoseconds> averageFrameTime);

}
