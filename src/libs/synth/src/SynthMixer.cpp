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
        // Equal-power panning would need a cosine.
        // Matching antwika::sound::Mixer matters more than evenness.
        // A voice moved between the two pools keeps its level.
        [[nodiscard]] float leftGainOf(float gain, float pan) noexcept
        {
            return gain * (1.0F - std::max(0.0F, pan));
        }

        [[nodiscard]] float rightGainOf(float gain, float pan) noexcept
        {
            return gain * (1.0F + std::min(0.0F, pan));
        }
    } // namespace

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

        // Sized once, here, and never resized.
        // That is what keeps render() free of allocation.
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

        if (isPeriodic(desc.shape) && !(desc.frequency > 0.0))
        {
            throw SynthError(
                "antwika::synth: a "
                + std::string(waveshapeName(desc.shape))
                + " voice needs a frequency above zero");
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

        // Every voice is busy, so one goes by a plain rotation.
        // That is how antwika::sound::Mixer steals too.
        // Deterministic rather than oldest-first, and stated.
        // Dropping the newest sound would look like a caller bug.
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

        const auto frequency = voice.desc.frequency
            + voice.desc.frequencySlide * seconds;

        const auto raw = oscillate(
            voice.desc.shape, voice.phase, voice.desc.seed, voice.elapsed);

        // A slide steep enough to pass zero holds at zero.
        // Running the phase backwards would sound like a reversal.
        voice.phase += std::max(0.0, frequency) / rate;
        voice.phase -= std::floor(voice.phase);

        const auto shaped = filterSample(
            voice.desc.filter.mode, voice.coefficients, voice.filter, raw);

        const auto level = envelopeAt(
            voice.desc.envelope, voice.elapsed, voice.desc.hold);

        // The gain is already folded into left and right.
        // Applying it here too would sound a voice at its square.
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

                // Not started yet: silence until its moment arrives.
                // That is what makes placement exact.
                // The alternative starts it at a buffer boundary.
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

} // namespace antwika::synth
