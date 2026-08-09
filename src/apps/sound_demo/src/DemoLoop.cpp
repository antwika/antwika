#include "antwika/sound_demo/DemoLoop.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>

#include <antwika/sound/Mixer.hpp>

namespace antwika::sound_demo
{

    using antwika::sound::Mixer;
    using antwika::sound::MixerDesc;

    namespace
    {
        constexpr FrameCount kLeadFrames = 4096;

        constexpr std::size_t kVoices = 16;

        constexpr std::size_t kDrainRounds = 100;

        [[nodiscard]] std::chrono::milliseconds millisecondsFor(
            FrameCount frames, std::uint32_t rate)
        {
            return std::chrono::milliseconds{
                static_cast<long long>(frames) * 1000 / rate};
        }
    }

    DemoLoop::DemoLoop(
        ISoundBackend &backend,
        const WaveformLibrary &library,
        ISleeper &sleeper)
        : backend(backend), library(library), sleeper(sleeper)
    {
    }

    void DemoLoop::run(
        const DeviceDesc &desc,
        const std::vector<PlayRequest> &notes,
        FrameCount frames)
    {
        const auto device = backend.openDevice(desc);

        const auto wave = device->format();

        Mixer mixer{
            library, MixerDesc{.format = wave, .maxVoices = kVoices}};

        for (const auto &note : notes)
        {
            mixer.play(note);
        }

        device->start(mixer);

        while (renderedFrames < frames)
        {
            const auto chunk = std::min<FrameCount>(
                device->bufferFrames(), frames - renderedFrames);

            renderedFrames += device->pump(chunk);

            const auto queued = renderedFrames - device->framesPlayed();

            if (queued > kLeadFrames)
            {
                sleeper.sleep(
                    millisecondsFor(queued - kLeadFrames, wave.rate));
            }
        }

        for (std::size_t round = 0; round < kDrainRounds; ++round)
        {
            const auto played = device->framesPlayed();

            if (played >= renderedFrames)
            {
                break;
            }

            sleeper.sleep(
                std::max(
                    std::chrono::milliseconds{1},
                    millisecondsFor(renderedFrames - played, wave.rate)));
        }

        device->stop();
    }

    FrameCount DemoLoop::rendered() const noexcept
    {
        return renderedFrames;
    }

}
