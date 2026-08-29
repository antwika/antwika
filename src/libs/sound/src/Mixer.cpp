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

    Mixer::Mixer(const WaveformLibrary &library, const MixerSpec &spec)
        : library(library), wave(spec.format)
    {
        if (!wave.isValid())
        {
            throw SoundError(
                "antwika::sound: a mixer cannot run at "
                + std::to_string(wave.rate) + " Hz with "
                + std::to_string(wave.channels) + " channels");
        }

        if (wave.channels > kStereoCount)
        {
            throw SoundError(
                "antwika::sound: a mixer pans stereo only, and cannot run "
                "with " + std::to_string(wave.channels) + " channels");
        }

        if (spec.maxVoices == 0)
        {
            throw SoundError(
                "antwika::sound: a mixer with no voices could never play "
                "anything");
        }

        voices.resize(spec.maxVoices);
    }

    void Mixer::play(const PlayRequest &request)
    {
        const auto &waveform = library.getWaveform(request.waveform);

        if (waveform.format.rate != wave.rate)
        {
            throw SoundError(
                "antwika::sound: a waveform at "
                + std::to_string(waveform.format.rate)
                + " Hz cannot be played by a mixer running at "
                + std::to_string(wave.rate)
                + " Hz, and this library does not resample");
        }

        const auto foundVoice = std::ranges::find_if(
            voices, [](const Voice &voice) { return !voice.active; });

        auto &voice =
            foundVoice != voices.end() ? *foundVoice : voices[nextVoiceToSteal];

        if (foundVoice == voices.end())
        {
            nextVoiceToSteal = (nextVoiceToSteal + 1) % voices.size();
        }

        voice = Voice{
            .sourceWaveform = &waveform,
            .startFrame = request.startFrame,
            .cursorCount = 0,
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

    std::size_t Mixer::getActiveVoices() const noexcept
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            voices, [](const Voice &voice) { return voice.active; }));
    }

    void Mixer::render(SampleBuffer samples, FrameIndex firstFrame) noexcept
    {
        samples.silence();

        const auto channels = samples.getChannelCount();

        for (auto &voice : voices)
        {
            if (!voice.active)
            {
                continue;
            }

            const auto &source = *voice.sourceWaveform;
            const auto length = source.getFrameCount();

            for (FrameCount frame = 0; frame < samples.frames; ++frame)
            {
                const auto frameIndex = firstFrame + frame;

                if (frameIndex < voice.startFrame)
                {
                    continue;
                }

                if (voice.cursorCount >= length)
                {
                    if (!voice.looping)
                    {
                        voice.active = false;
                        break;
                    }

                    voice.cursorCount = 0;
                }

                for (ChannelCount channel = 0; channel < channels;
                     ++channel)
                {
                    const auto lane = channel % source.format.channels;

                    const auto sample =
                        source.samples
                            [voice.cursorCount * source.format.channels + lane];

                    const auto gain = channel == 0 ? voice.left
                                    : voice.right;

                    samples.channels[channel][frame] += sample * gain;
                }

                ++voice.cursorCount;
            }
        }
    }

}
