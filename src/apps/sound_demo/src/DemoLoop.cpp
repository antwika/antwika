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
        // How far ahead of the hardware the queue may run.
        // Long enough that a late pump is inaudible.
        // Short enough that the program outlives what it queued.
        constexpr FrameCount kLeadFrames = 4096;

        constexpr std::size_t kVoices = 16;

        // How many times the drain wait goes round before giving up.
        // Generous next to any real buffer, and still finite.
        // A device that stopped consuming ends the run, not hangs it.
        constexpr std::size_t kDrainRounds = 100;

        [[nodiscard]] std::chrono::milliseconds millisecondsFor(
            FrameCount frames, std::uint32_t rate)
        {
            return std::chrono::milliseconds{
                static_cast<long long>(frames) * 1000 / rate};
        }
    } // namespace

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

        // The device's own format throughout, never the one asked for.
        // A backend may open something else and say so.
        // Pacing on the request would use a rate nothing plays at.
        const auto wave = device->format();

        Mixer mixer{
            library, MixerDesc{.format = wave, .maxVoices = kVoices}};

        // Every note is handed over before a frame is rendered.
        // That is what an absolute start frame buys.
        // The schedule is decided once, not chased by the pumping.
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

            // The one legal reading of framesPlayed(): how long to wait.
            // Nothing computed above depends on it.
            // A device reporting it differently changes only pacing.
            const auto queued = renderedFrames - device->framesPlayed();

            if (queued > kLeadFrames)
            {
                sleeper.sleep(
                    millisecondsFor(queued - kLeadFrames, wave.rate));
            }
        }

        // Wait for what is queued to be heard before closing.
        // Otherwise the track's last second is discarded on exit.
        //
        // Bounded, since this loop's exit condition is the device's.
        // A device that stopped consuming would otherwise hang the run.
        // Ending a little early is much the better failure here.
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

} // namespace antwika::sound_demo
