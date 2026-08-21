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
        IRenderCallback &sinkCallback,
        ChannelCount channels,
        FrameCount frameCount,
        FrameCount frames,
        FrameIndex firstFrame,
        OnChunk onChunk)
    {
        Planes planes(channels, std::vector<float>(frameCount, 0.0F));

        std::vector<std::span<float>> views;
        views.reserve(planes.size());

        for (auto &plane : planes)
        {
            views.emplace_back(plane);
        }

        FrameCount doneCount = 0;

        while (doneCount < frames)
        {
            const auto chunk = std::min<FrameCount>(
                frameCount,
                frames - doneCount);

            sinkCallback.render(
                SampleBuffer{.channels = views, .frames = chunk},
                firstFrame + doneCount);

            onChunk(planes, chunk);

            doneCount += chunk;
        }

        return doneCount;
    }

}
