#include "Sdl3Device.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>

#include <antwika/log/Level.hpp>
#include <antwika/sound/SampleBuffer.hpp>
#include <antwika/sound/SoundError.hpp>

namespace antwika::sdl3
{

    using antwika::log::Level;
    using antwika::sound::SampleBuffer;
    using antwika::sound::SoundError;

    namespace
    {
        constexpr FrameCount kFallbackBufferFrames = 1024;

        [[nodiscard]] std::size_t bytesPerFrame(const WaveFormat &wave)
        {
            return static_cast<std::size_t>(wave.channels) * sizeof(float);
        }
    }

    Sdl3Device::Sdl3Device(ILogger &logger, const DeviceDesc &desc)
        : audio(logger, SDL_INIT_AUDIO, "audio"),
          wave(desc.format),
          buffer(
              desc.preferredBufferFrames > 0 ? desc.preferredBufferFrames
                                             : kFallbackBufferFrames)
    {
        SDL_AudioSpec spec{};
        spec.format = SDL_AUDIO_F32;
        spec.channels = static_cast<int>(wave.channels);
        spec.freq = static_cast<int>(wave.rate);

        stream = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);

        if (stream == nullptr)
        {
            throw Sdl3Error(
                std::string("sdl3: could not open an audio device: ")
                + SDL_GetError());
        }

        if (!SDL_ResumeAudioStreamDevice(stream))
        {
            const std::string what = SDL_GetError();
            SDL_DestroyAudioStream(stream);
            stream = nullptr;

            throw Sdl3Error(
                std::string("sdl3: could not start an audio device: ")
                + what);
        }

        planes.assign(
            wave.channels, std::vector<float>(buffer, 0.0F));
        interleaved.assign(
            static_cast<std::size_t>(buffer) * wave.channels, 0.0F);

        logger.log(
            Level::Debug,
            "sound.sdl3: device open at " + std::to_string(wave.rate)
                + " Hz");
    }

    Sdl3Device::~Sdl3Device()
    {
        sink = nullptr;

        if (stream != nullptr)
        {
            SDL_DestroyAudioStream(stream);
        }
    }

    void Sdl3Device::start(IRenderCallback &callback)
    {
        if (sink != nullptr)
        {
            throw SoundError(
                "sound.sdl3: the device is already started");
        }

        sink = &callback;
    }

    void Sdl3Device::stop()
    {
        sink = nullptr;
    }

    FrameCount Sdl3Device::pump(FrameCount frames)
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

    void Sdl3Device::render(FrameCount frames)
    {
        std::vector<std::span<float>> views;
        views.reserve(planes.size());

        for (auto &plane : planes)
        {
            views.emplace_back(plane.data(), frames);
        }

        sink->render(
            SampleBuffer{.channels = views, .frames = frames}, pushed);

        for (FrameCount frame = 0; frame < frames; ++frame)
        {
            for (std::size_t channel = 0; channel < planes.size();
                 ++channel)
            {
                interleaved[frame * planes.size() + channel] =
                    planes[channel][frame];
            }
        }

        const auto bytes = static_cast<int>(frames * bytesPerFrame(wave));

        if (!SDL_PutAudioStreamData(stream, interleaved.data(), bytes))
        {
            SDL_ClearError();
        }

        pushed += frames;
    }

    WaveFormat Sdl3Device::format() const
    {
        return wave;
    }

    FrameCount Sdl3Device::bufferFrames() const
    {
        return buffer;
    }

    FrameIndex Sdl3Device::framesPlayed() const
    {
        const auto queuedBytes = SDL_GetAudioStreamQueued(stream);

        const auto queued = queuedBytes <= 0
            ? FrameIndex{0}
            : static_cast<FrameIndex>(
                  static_cast<std::size_t>(queuedBytes)
                  / bytesPerFrame(wave));

        const auto consumed = queued >= pushed ? FrameIndex{0}
                                               : pushed - queued;

        played = std::max(played, consumed);

        return played;
    }

}
