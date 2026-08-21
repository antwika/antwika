#pragma once

#include <cstdint>

namespace antwika::sound
{

    using SampleRate = std::uint32_t;

    inline constexpr SampleRate kDefaultSampleRate = 48000;

    using ChannelCount = std::uint8_t;

    inline constexpr ChannelCount kMonoCount = 1;
    inline constexpr ChannelCount kStereoCount = 2;

    inline constexpr ChannelCount kMaxChannels = 8;

    struct WaveFormat final
    {
        SampleRate rate = kDefaultSampleRate;
        ChannelCount channels = kStereoCount;

        [[nodiscard]] bool isValid() const noexcept;

        [[nodiscard]] bool operator==(const WaveFormat &other) const
            = default;
    };

}
