#pragma once

#include <algorithm>
#include <span>
#include <vector>

#include "antwika/sound/Frames.hpp"
#include "antwika/sound/IRenderCallback.hpp"
#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/WaveFormat.hpp"

namespace antwika::sound::detail
{

    using Planes = std::vector<std::vector<float>>;

    template <typename OnChunk>
    FrameCount renderInChunks(
        IRenderCallback &sink,
        ChannelCount channels,
        FrameCount buffer,
        FrameCount frames,
        FrameIndex firstFrame,
        OnChunk onChunk)
    {
        Planes planes(channels, std::vector<float>(buffer, 0.0F));

        std::vector<std::span<float>> views;
        views.reserve(planes.size());

        for (auto &plane : planes)
        {
            views.emplace_back(plane);
        }

        FrameCount done = 0;

        while (done < frames)
        {
            const auto chunk = std::min<FrameCount>(buffer, frames - done);

            sink.render(
                SampleBuffer{.channels = views, .frames = chunk},
                firstFrame + done);

            onChunk(planes, chunk);

            done += chunk;
        }

        return done;
    }

}
