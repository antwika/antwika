#include "antwika/sound/Mixer.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    namespace
    {
        // Equal-power panning would need a cosine.
        // Exact arithmetic is worth more here than perceptual evenness.
        [[nodiscard]] float leftGainOf(float gain, float pan) noexcept
        {
            return gain * (1.0F - std::max(0.0F, pan));
        }

        [[nodiscard]] float rightGainOf(float gain, float pan) noexcept
        {
            return gain * (1.0F + std::min(0.0F, pan));
        }
    } // namespace

    Mixer::Mixer(const WaveformLibrary &library, const MixerDesc &desc)
        : library(library), wave(desc.format)
    {
        if (!wave.isValid())
        {
            throw SoundError(
                "antwika::sound: a mixer cannot run at "
                + std::to_string(wave.rate) + " Hz with "
                + std::to_string(wave.channels) + " channels");
        }

        if (desc.maxVoices == 0)
        {
            throw SoundError(
                "antwika::sound: a mixer with no voices could never play "
                "anything");
        }

        // Sized once, here, and never resized.
        // That is what keeps render() free of allocation.
        voices.resize(desc.maxVoices);
    }

    void Mixer::play(const PlayRequest &request)
    {
        const auto &waveform = library.get(request.waveform);

        if (waveform.format.rate != wave.rate)
        {
            throw SoundError(
                "antwika::sound: a waveform at "
                + std::to_string(waveform.format.rate)
                + " Hz cannot be played by a mixer running at "
                + std::to_string(wave.rate)
                + " Hz, and this library does not resample");
        }

        const auto found = std::ranges::find_if(
            voices, [](const Voice &voice) { return !voice.active; });

        // Every voice is busy, so one goes by a plain rotation.
        // The cursor moves on only when a steal happens.
        // It names the voice after the last one stolen from.
        // That is not the oldest voice, and does not claim to be.
        // The oldest would need a sequence number kept per voice.
        // A rotation is already deterministic, which is what this needs.
        // Stealing is stated rather than left to chance.
        // Silently dropping the newest sound would look like a caller bug.
        auto &voice =
            found != voices.end() ? *found : voices[nextSteal];

        if (found == voices.end())
        {
            nextSteal = (nextSteal + 1) % voices.size();
        }

        voice = Voice{
            .source = &waveform,
            .startFrame = request.startFrame,
            .cursor = 0,
            .left = leftGainOf(request.gain, request.pan),
            .right = rightGainOf(request.gain, request.pan),
            .looping = request.looping,
            .active = true};
    }

    void Mixer::stopAll() noexcept
    {
        for (auto &voice : voices)
        {
            voice.active = false;
        }
    }

    std::size_t Mixer::activeVoices() const noexcept
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            voices, [](const Voice &voice) { return voice.active; }));
    }

    void Mixer::render(SampleBuffer out, FrameIndex firstFrame) noexcept
    {
        out.silence();

        const auto channels = out.channelCount();

        for (auto &voice : voices)
        {
            if (!voice.active)
            {
                continue;
            }

            const auto &source = *voice.source;
            const auto length = source.frameCount();

            for (FrameCount frame = 0; frame < out.frames; ++frame)
            {
                const auto at = firstFrame + frame;

                // Not started yet: silence until its moment arrives.
                // That is what makes placement exact.
                // The alternative starts it at the next buffer boundary.
                if (at < voice.startFrame)
                {
                    continue;
                }

                if (voice.cursor >= length)
                {
                    if (!voice.looping)
                    {
                        voice.active = false;
                        break;
                    }

                    // The library refuses a waveform holding no frames.
                    // Every voice is resolved through it by play().
                    // So a restart always has a sample to read.
                    voice.cursor = 0;
                }

                for (ChannelCount channel = 0; channel < channels;
                     ++channel)
                {
                    // A mono source feeds every channel.
                    // A wider one is read lane for lane, extras dropped.
                    const auto lane = channel % source.format.channels;

                    const auto sample =
                        source.samples
                            [voice.cursor * source.format.channels + lane];

                    const auto gain = channel == 0 ? voice.left
                                                   : voice.right;

                    out.channels[channel][frame] += sample * gain;
                }

                ++voice.cursor;
            }
        }
    }

} // namespace antwika::sound
