#include "antwika/sound/NullDevice.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    NullDevice::NullDevice(const DeviceDesc &desc)
        : wave(desc.format),
          buffer(
              desc.preferredBufferFrames > 0 ? desc.preferredBufferFrames
                                             : kDefaultBufferFrames)
    {
    }

    void NullDevice::start(IRenderCallback &callback)
    {
        if (sink != nullptr)
        {
            throw SoundError(
                "antwika::sound: the device is already started");
        }

        sink = &callback;
    }

    void NullDevice::stop()
    {
        sink = nullptr;
    }

    FrameCount NullDevice::pump(FrameCount frames)
    {
        if (sink == nullptr)
        {
            return 0;
        }

        // One scratch buffer per call rather than one per device.
        // Nothing here is on a real-time path.
        // A clock would be state a test has to reason about.
        std::vector<std::vector<float>> planes(
            wave.channels, std::vector<float>(buffer, 0.0F));

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

            sink->render(
                SampleBuffer{.channels = views, .frames = chunk},
                played + done);

            done += chunk;
        }

        played += done;
        return done;
    }

    WaveFormat NullDevice::format() const
    {
        return wave;
    }

    FrameCount NullDevice::bufferFrames() const
    {
        return buffer;
    }

    FrameIndex NullDevice::framesPlayed() const
    {
        return played;
    }

} // namespace antwika::sound
