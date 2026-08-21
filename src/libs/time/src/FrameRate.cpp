#include "antwika/time/FrameRate.hpp"

#include <cmath>

namespace antwika::time
{

    namespace
    {
        constexpr float kASecond = 1'000'000'000.0F;

        constexpr double kATenth = 100'000.0;

    }

    void FrameRate::record(const std::chrono::nanoseconds span)
    {
        if (span.count() <= 0)
        {
            return;
        }

        spans.at(slotIndex) = span;
        slotIndex = (slotIndex + 1) % kFrameSampleCount;
        sampleCount =
            sampleCount < kFrameSampleCount ? sampleCount + 1 : sampleCount;
    }

    std::optional<std::chrono::nanoseconds> FrameRate::averageFrameTime()
        const
    {
        if (sampleCount == 0)
        {
            return std::nullopt;
        }

        auto totalNanoseconds = std::chrono::nanoseconds::rep{0};

        for (std::size_t sample = 0; sample < sampleCount; ++sample)
        {
            totalNanoseconds += spans.at(sample).count();
        }

        return std::chrono::nanoseconds{
            totalNanoseconds / static_cast<std::chrono::nanoseconds::rep>(
                        sampleCount)};
    }

    std::optional<float> FrameRate::perSecond() const
    {
        const auto each = averageFrameTime();

        if (!each.has_value())
        {
            return std::nullopt;
        }

        return kASecond / static_cast<float>(each->count());
    }

    std::string formatFrameRate(const std::optional<float> perSecond)
    {
        if (!perSecond.has_value())
        {
            return "- fps";
        }

        return std::to_string(
                   static_cast<int>(std::lround(*perSecond)))
               + " fps";
    }

    std::string formatFrameTime(
        const std::optional<std::chrono::nanoseconds> averageFrameTime)
    {
        if (!averageFrameTime.has_value())
        {
            return "- ms";
        }

        const auto tenths = std::lround(
            static_cast<double>(averageFrameTime->count()) / kATenth);

        return std::to_string(tenths / 10) + "."
               + std::to_string(tenths % 10) + " ms";
    }

}
