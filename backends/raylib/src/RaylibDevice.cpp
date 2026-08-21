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

        constexpr FrameCount kSubBuffersCount = 2;
    }

    RaylibDevice::RaylibDevice(ILogger &logger, const DeviceSpec &spec)
        : audio(RaylibAudioRuntime::acquire(logger)),
          wave(spec.format),
          bufferCount(
              spec.preferredBufferFrames > 0 ? spec.preferredBufferFrames
                                             : kFallbackBufferFrames)
    {
        channelBuffers.assign(
            wave.channels,
            std::vector<float>(bufferCount, 0.0F));

        if (!audio->isReady())
        {
            return;
        }

        SetAudioStreamBufferSizeDefault(static_cast<int>(bufferCount));

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
        sinkCallback = nullptr;

        if (streaming)
        {
            StopAudioStream(stream);
            UnloadAudioStream(stream);
        }
    }

    void RaylibDevice::start(IRenderCallback &callback)
    {
        if (sinkCallback != nullptr)
        {
            throw SoundError("sound.raylib: the device is already started");
        }

        sinkCallback = &callback;
    }

    void RaylibDevice::stop()
    {
        sinkCallback = nullptr;
    }

    FrameCount RaylibDevice::advance(FrameCount frames)
    {
        if (sinkCallback == nullptr)
        {
            return 0;
        }

        FrameCount doneCount = 0;

        while (
            doneCount < frames)
        {
            const auto chunk = std::min<FrameCount>(
                bufferCount,
                frames - doneCount);

            render(chunk);
            doneCount += chunk;
        }

        return doneCount;
    }

    void RaylibDevice::render(FrameCount frames)
    {
        std::vector<std::span<float>> views;
        views.reserve(channelBuffers.size());

        for (auto &channelBuffer : channelBuffers)
        {
            views.emplace_back(channelBuffer.data(), frames);
        }

        sinkCallback->render(
            SampleBuffer{.channels = views, .frames = frames}, renderedIndex);

        renderedIndex += frames;

        if (!streaming)
        {
            acceptedIndex += frames;

            return;
        }

        for (FrameCount frame = 0; frame < frames; ++frame)
        {
            for (std::size_t channel = 0; channel < channelBuffers.size();
                 ++channel)
            {
                pending.push_back(channelBuffers[channel][frame]);
            }
        }

        flushToStream();
    }

    FrameCount RaylibDevice::pendingFrames() const noexcept
    {
        return static_cast<FrameCount>(
            (pending.size() - pendingRead) / channelBuffers.size());
    }

    void RaylibDevice::flushToStream()
    {
        while (pendingFrames() >= bufferCount && IsAudioStreamProcessed(stream))
        {
            UpdateAudioStream(
                stream,
                pending.data() + pendingRead,
                static_cast<int>(bufferCount));

            pendingRead += static_cast<std::size_t>(bufferCount)
                           * channelBuffers.size();
            acceptedIndex += bufferCount;
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
        return bufferCount;
    }

    FrameIndex RaylibDevice::framesPlayed() const
    {
        FrameCount bufferedFrames = 0;

        if (streaming)
        {
            bufferedFrames = IsAudioStreamProcessed(stream)
                           ? bufferCount
                           : bufferCount * kSubBuffersCount;
        }

        const auto safeIndex =
            std::min<FrameIndex>(acceptedIndex, bufferedFrames);

        playedIndex = std::max(playedIndex, acceptedIndex - safeIndex);

        return playedIndex;
    }

}
