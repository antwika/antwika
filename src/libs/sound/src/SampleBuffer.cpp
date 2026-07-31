#include "antwika/sound/SampleBuffer.hpp"

#include <algorithm>
#include <cstddef>

namespace antwika::sound
{

    ChannelCount SampleBuffer::channelCount() const noexcept
    {
        return static_cast<ChannelCount>(channels.size());
    }

    bool SampleBuffer::isComplete() const noexcept
    {
        if (channels.empty() || channels.size() > kMaxChannels)
        {
            return false;
        }

        return std::ranges::all_of(
            channels,
            [this](std::span<float> channel)
            { return channel.size() >= frames; });
    }

    void SampleBuffer::silence() const noexcept
    {
        for (const auto channel : channels)
        {
            std::ranges::fill(channel.first(frames), 0.0F);
        }
    }

} // namespace antwika::sound
