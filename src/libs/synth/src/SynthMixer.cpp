#include "antwika/synth/SynthMixer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

#include <antwika/sound/WaveFormat.hpp>

#include "antwika/synth/Adsr.hpp"
#include "antwika/synth/Filter.hpp"
#include "antwika/synth/Oscillate.hpp"
#include "antwika/synth/SynthError.hpp"
#include "antwika/synth/Waveshape.hpp"

namespace antwika::synth
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

    SynthMixer::SynthMixer(const SynthMixerDesc &desc)
        : wave(desc.format)
    {
        if (!wave.isValid())
        {
            throw SynthError(
                "antwika::synth: a mixer cannot run at "
                + std::to_string(wave.rate) + " Hz with "
                + std::to_string(wave.channels) + " channels");
        }

        if (desc.maxVoices == 0)
        {
            throw SynthError(
                "antwika::synth: a mixer with no voices could never "
                "sound anything");
        }

        voices.resize(desc.maxVoices);
    }

    void SynthMixer::trigger(const TriggerRequest &request)
    {
        const auto &desc = request.voice;

        if (desc.totalFrames() == 0)
        {
            throw SynthError(
                "antwika::synth: a voice holding for no frames and "
                "releasing over none would never be heard");
        }

        if (isPeriodic(desc.shape)
            && (!(desc.frequency > 0.0)
                || !std::isfinite(desc.frequency)))
        {
            throw SynthError(
                "antwika::synth: a "
                + std::string(waveshapeName(desc.shape))
                + " voice needs a finite frequency above zero");
        }

        if (desc.vibratoHertz < 0.0
            || !std::isfinite(desc.vibratoHertz)
            || desc.vibratoHertz >= static_cast<double>(wave.rate) / 2.0)
        {
            throw SynthError(
                "antwika::synth: a vibrato of "
                + std::to_string(desc.vibratoHertz)
                + " Hz is not between zero and half the sample rate");
        }

        if (desc.vibratoDepth < 0.0 || desc.vibratoDepth >= 1.0
            || !std::isfinite(desc.vibratoDepth))
        {
            throw SynthError(
                "antwika::synth: a vibrato depth of "
                + std::to_string(desc.vibratoDepth)
                + " lies outside zero up to one");
        }

        if (!(desc.arpeggioRatio > 0.0)
            || !std::isfinite(desc.arpeggioRatio))
        {
            throw SynthError(
                "antwika::synth: an arpeggio ratio of "
                + std::to_string(desc.arpeggioRatio)
                + " is not a finite ratio above zero");
        }

        if (desc.envelope.sustain < 0.0F || desc.envelope.sustain > 1.0F)
        {
            throw SynthError(
                "antwika::synth: a sustain of "
                + std::to_string(desc.envelope.sustain)
                + " lies outside zero to one");
        }

        if (desc.filter.mode != FilterMode::None)
        {
            const auto nyquist = static_cast<double>(wave.rate) / 2.0;

            if (!(desc.filter.cutoff > 0.0)
                || desc.filter.cutoff >= nyquist)
            {
                throw SynthError(
                    "antwika::synth: a filter cutoff of "
                    + std::to_string(desc.filter.cutoff)
                    + " Hz is not below half of "
                    + std::to_string(wave.rate) + " Hz");
            }

            if (!(desc.filter.resonance > 0.0))
            {
                throw SynthError(
                    "antwika::synth: a resonance of "
                    + std::to_string(desc.filter.resonance)
                    + " leaves the filter undamped, so it would ring "
                      "on rather than decay");
            }
        }

        const auto found = std::ranges::find_if(
            voices, [](const Voice &voice) { return !voice.active; });

        auto &voice = found != voices.end() ? *found : voices[nextSteal];

        if (found == voices.end())
        {
            nextSteal = (nextSteal + 1) % voices.size();
        }

        voice = Voice{
            .desc = desc,
            .coefficients = filterCoefficientsFor(desc.filter, wave.rate),
            .filter = FilterState{},
            .startFrame = request.startFrame,
            .elapsed = 0,
            .phase = 0.0,
            .left = leftGainOf(desc.gain, desc.pan),
            .right = rightGainOf(desc.gain, desc.pan),
            .active = true};
    }

    void SynthMixer::stopAll() noexcept
    {
        for (auto &voice : voices)
        {
            voice.active = false;
        }
    }

    std::size_t SynthMixer::activeVoices() const noexcept
    {
        return static_cast<std::size_t>(std::ranges::count_if(
            voices, [](const Voice &voice) { return voice.active; }));
    }

    WaveFormat SynthMixer::format() const noexcept
    {
        return wave;
    }

    float SynthMixer::nextSample(Voice &voice) const noexcept
    {
        const auto rate = static_cast<double>(wave.rate);
        const auto seconds = static_cast<double>(voice.elapsed) / rate;

        auto frequency = voice.desc.frequency
            + voice.desc.frequencySlide * seconds;

        if (voice.desc.vibratoHertz > 0.0)
        {
            const auto turn =
                voice.vibratoPhase - std::floor(voice.vibratoPhase);
            const auto triangle = 4.0 * std::abs(turn - 0.5) - 1.0;

            frequency *= 1.0 + voice.desc.vibratoDepth * triangle;
            voice.vibratoPhase += voice.desc.vibratoHertz / rate;
        }

        if (voice.desc.arpeggioPeriod > 0
            && (voice.elapsed / voice.desc.arpeggioPeriod) % 2 == 1)
        {
            frequency *= voice.desc.arpeggioRatio;
        }

        const auto raw = oscillate(
            voice.desc.shape, voice.phase, voice.desc.seed, voice.elapsed);

        voice.phase += std::max(0.0, frequency) / rate;
        voice.phase -= std::floor(voice.phase);

        const auto shaped = filterSample(
            voice.desc.filter.mode, voice.coefficients, voice.filter, raw);

        const auto level = envelopeAt(
            voice.desc.envelope, voice.elapsed, voice.desc.hold);

        return shaped * level;
    }

    void SynthMixer::render(
        SampleBuffer out, FrameIndex firstFrame) noexcept
    {
        out.silence();

        const auto channels = out.channelCount();

        for (auto &voice : voices)
        {
            if (!voice.active)
            {
                continue;
            }

            const auto total = voice.desc.totalFrames();

            for (FrameCount frame = 0; frame < out.frames; ++frame)
            {
                const auto at = firstFrame + frame;

                if (at < voice.startFrame)
                {
                    continue;
                }

                if (voice.elapsed >= total)
                {
                    voice.active = false;
                    break;
                }

                const auto sample = nextSample(voice);

                for (sound::ChannelCount channel = 0;
                     channel < channels; ++channel)
                {
                    const auto gain =
                        channel == 0 ? voice.left : voice.right;

                    out.channels[channel][frame] += sample * gain;
                }

                ++voice.elapsed;
            }
        }
    }

}
