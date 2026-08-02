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

    /** @brief A device's scratch buffer: one vector per channel. */
    using Planes = std::vector<std::vector<float>>;

    /**
     * @brief Render a run of frames through a callback, a chunk at a
     * time.
     *
     * The scaffolding a pumped device needs and none of what it then
     * does with the result: a planar scratch buffer, the spans over it
     * that a SampleBuffer is made of, and a loop clamping the last chunk
     * to whatever is left. NullDevice and OfflineDevice differ only in
     * what happens to a chunk once the sink has filled it, so that
     * difference is the callback and everything else is here -- which is
     * what stops a change to the clamp from being visible to one
     * device's tests and invisible to the other's.
     *
     * One scratch buffer per call rather than one per device, because
     * nothing here is on a real-time path and a buffer that outlived a
     * call would be state a test has to reason about.
     *
     * @param sink What fills each chunk; the caller has already checked
     * that there is one.
     * @param channels How many planes to render into.
     * @param buffer The most frames one chunk may hold; never zero, or
     * this would render nothing for ever.
     * @param frames How many frames to render in total.
     * @param firstFrame The absolute index of the first frame rendered,
     * which is what the sink is handed for the first chunk.
     * @param onChunk Called with the planes and that chunk's frame count
     * once the sink has filled it, for every chunk, in order.
     * @return How many frames were rendered, which is `frames`.
     */
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

} // namespace antwika::sound::detail
