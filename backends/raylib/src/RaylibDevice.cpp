#include "RaylibDevice.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>

#include <antwika/log/Level.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/sound/SoundError.hpp>

namespace antwika::raylib
{

    using antwika::log::Level;
    using antwika::sound::SampleBuffer;
    using antwika::sound::SoundError;

    namespace
    {
        constexpr FrameCount kFallbackBufferFrames = 1024;

        constexpr unsigned int kBitsPerSample = 32;

        constexpr FrameCount kSubBuffers = 2;
    }

    RaylibDevice::RaylibDevice(ILogger &logger, const DeviceDesc &desc)
        : audio(RaylibAudioRuntime::acquire(logger)),
          wave(desc.format),
          buffer(
              desc.preferredBufferFrames > 0 ? desc.preferredBufferFrames
                                             : kFallbackBufferFrames)
    {
        planes.assign(wave.channels, std::vector<float>(buffer, 0.0F));

        if (!audio->isReady())
        {
            return;
        }

        SetAudioStreamBufferSizeDefault(static_cast<int>(buffer));

        stream = LoadAudioStream(wave.rate, kBitsPerSample, wave.channels);

        if (!IsAudioStreamValid(stream))
        {
            throw RaylibError("raylib: could not open an audio stream");
        }

        streaming = true;

        PlayAudioStream(stream);

        logger.log(
            Level::Debug,
            "sound.raylib: device open at " + std::to_string(wave.rate)
                + " Hz");
    }

    RaylibDevice::~RaylibDevice()
    {
        sink = nullptr;

        if (streaming)
        {
            StopAudioStream(stream);
            UnloadAudioStream(stream);
        }
    }

    void RaylibDevice::start(IRenderCallback &callback)
    {
        if (sink != nullptr)
        {
            throw SoundError("sound.raylib: the device is already started");
        }

        sink = &callback;
    }

    void RaylibDevice::stop()
    {
        sink = nullptr;
    }

    FrameCount RaylibDevice::pump(FrameCount frames)
    {
        if (sink == nullptr)
        {
            return 0;
        }

        FrameCount done = 0;

        while (done < frames)
        {
            const auto chunk = std::min<FrameCount>(buffer, frames - done);

            render(chunk);
            done += chunk;
        }

        return done;
    }

    void RaylibDevice::render(FrameCount frames)
    {
        std::vector<std::span<float>> views;
        views.reserve(planes.size());

        for (auto &plane : planes)
        {
            views.emplace_back(plane.data(), frames);
        }

        sink->render(
            SampleBuffer{.channels = views, .frames = frames}, rendered);

        rendered += frames;

        if (!streaming)
        {
            accepted += frames;

            return;
        }

        for (FrameCount frame = 0; frame < frames; ++frame)
        {
            for (std::size_t channel = 0; channel < planes.size();
                 ++channel)
            {
                pending.push_back(planes[channel][frame]);
            }
        }

        drain();
    }

    FrameCount RaylibDevice::pendingFrames() const noexcept
    {
        return static_cast<FrameCount>(
            (pending.size() - pendingRead) / planes.size());
    }

    void RaylibDevice::drain()
    {
        while (pendingFrames() >= buffer && IsAudioStreamProcessed(stream))
        {
            UpdateAudioStream(
                stream,
                pending.data() + pendingRead,
                static_cast<int>(buffer));

            pendingRead += static_cast<std::size_t>(buffer) * planes.size();
            accepted += buffer;
        }

        if (pendingRead == pending.size())
        {
            pending.clear();
            pendingRead = 0;

            return;
        }

        if (pendingRead >= pending.size() / 2)
        {
            pending.erase(
                pending.begin(),
                pending.begin() + static_cast<std::ptrdiff_t>(pendingRead));
            pendingRead = 0;
        }
    }

    WaveFormat RaylibDevice::format() const
    {
        return wave;
    }

    FrameCount RaylibDevice::bufferFrames() const
    {
        return buffer;
    }

    FrameIndex RaylibDevice::framesPlayed() const
    {
        FrameCount held = 0;

        if (streaming)
        {
            held = IsAudioStreamProcessed(stream) ? buffer
                                                  : buffer * kSubBuffers;
        }

        const auto behind = std::min<FrameIndex>(accepted, held);

        played = std::max(played, accepted - behind);

        return played;
    }

}
