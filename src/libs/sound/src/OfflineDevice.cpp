#include "antwika/sound/OfflineDevice.hpp"

#include "antwika/sound/SoundError.hpp"
#include "RenderInChunks.hpp"

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

        // Planar in, interleaved out.
        // This is the one place the library crosses between them.
        const auto done = detail::renderInChunks(
            *sink,
            wave.channels,
            buffer,
            frames,
            played,
            [this](const detail::Planes &planes, FrameCount chunk)
            {
                for (FrameCount frame = 0; frame < chunk; ++frame)
                {
                    for (const auto &plane : planes)
                    {
                        out.samples.push_back(plane[frame]);
                    }
                }
            });

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
