#include "antwika/sound/OfflineDevice.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <vector>

#include "antwika/sound/SampleBuffer.hpp"
#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    OfflineDevice::OfflineDevice(const DeviceDesc &desc, Waveform &sink)
        : wave(desc.format),
          buffer(
              desc.preferredBufferFrames > 0 ? desc.preferredBufferFrames
                                             : kDefaultBufferFrames),
          out(sink)
    {
        out.format = wave;
    }

    void OfflineDevice::start(IRenderCallback &callback)
    {
        if (sink != nullptr)
        {
            throw SoundError(
                "antwika::sound: the device is already started");
        }

        sink = &callback;
    }

    void OfflineDevice::stop()
    {
        sink = nullptr;
    }

    FrameCount OfflineDevice::pump(FrameCount frames)
    {
        if (sink == nullptr)
        {
            return 0;
        }

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

            // Planar in, interleaved out.
            // This is the one place the library crosses between them.
            for (FrameCount frame = 0; frame < chunk; ++frame)
            {
                for (const auto &plane : planes)
                {
                    out.samples.push_back(plane[frame]);
                }
            }

            done += chunk;
        }

        played += done;
        return done;
    }

    WaveFormat OfflineDevice::format() const
    {
        return wave;
    }

    FrameCount OfflineDevice::bufferFrames() const
    {
        return buffer;
    }

    FrameIndex OfflineDevice::framesPlayed() const
    {
        return played;
    }

} // namespace antwika::sound
