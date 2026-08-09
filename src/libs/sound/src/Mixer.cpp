#include "antwika/sound/Mixer.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

#include "antwika/sound/SoundError.hpp"

namespace antwika::sound
{

    namespace
    {
        [[nodiscard]] float leftGainOf(float gain, float pan) noexcept
        {
            return gain * (1.0F - std::max(0.0F, pan));
        }

        [[nodiscard]] float rightGainOf(float gain, float pan) noexcept
        {
            return gain * (1.0F + std::min(0.0F, pan));
        }
    }

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

                    voice.cursor = 0;
                }

                for (ChannelCount channel = 0; channel < channels;
                     ++channel)
                {
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

}
