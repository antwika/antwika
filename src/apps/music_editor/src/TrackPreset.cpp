#include "antwika/music_editor/TrackPreset.hpp"

#include "ScoreText.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/pattern/Controls.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/synth/Adsr.hpp>
#include <antwika/synth/Filter.hpp>
#include <antwika/synth/SynthError.hpp>
#include <antwika/synth/TriggerRequest.hpp>
#include <antwika/synth/VoiceDesc.hpp>
#include <antwika/synth/Waveshape.hpp>

namespace antwika::music_editor
{

    namespace
    {
        // Gentle: half a per cent of the pitch either way.
        constexpr double kDefaultVibratoDepth = 0.005;

        // The classic chiptune rate: twenty-five steps a second.
        constexpr std::uint32_t kArpeggioStepMs = 40;
    } // namespace

    namespace
    {
        constexpr std::uint32_t kMillisecondsPerSecond = 1000;

        constexpr double kSemitones =
            static_cast<double>(detail::kSemitonesPerOctave);

        [[nodiscard]] FrameCount framesForMs(
            std::uint32_t milliseconds, SampleRate rate) noexcept
        {
            return static_cast<FrameCount>(milliseconds)
                * static_cast<FrameCount>(rate) / kMillisecondsPerSecond;
        }

        constexpr std::array<std::string_view, kPresetCount> kNames{
            "bass", "lead", "bell", "drum"};

        const std::array<TrackPreset, kPresetCount> kPresets{
            TrackPreset{
                .shape = synth::Waveshape::Saw,
                .baseHertz = 110.0,
                .attackMs = 4,
                .decayMs = 120,
                .sustain = 0.6F,
                .releaseMs = 90,
                .maxHoldMs = 400,
                .filter = synth::FilterDesc{
                    .mode = synth::FilterMode::LowPass,
                    .cutoff = 900.0,
                    .resonance = 0.8},
                .gain = 0.35F,
                .pan = -0.3F},
            TrackPreset{
                .shape = synth::Waveshape::Square,
                .baseHertz = 440.0,
                .attackMs = 2,
                .decayMs = 40,
                .sustain = 0.5F,
                .releaseMs = 70,
                .maxHoldMs = 250,
                .filter = synth::FilterDesc{
                    .mode = synth::FilterMode::LowPass,
                    .cutoff = 3000.0,
                    .resonance = 0.7},
                .gain = 0.22F,
                .pan = 0.3F},
            TrackPreset{
                .shape = synth::Waveshape::Sine,
                .baseHertz = 880.0,
                .attackMs = 1,
                .decayMs = 300,
                .sustain = 0.0F,
                .releaseMs = 400,
                .maxHoldMs = 1200,
                .gain = 0.3F,
                .pan = 0.0F},
            TrackPreset{
                .shape = synth::Waveshape::Noise,
                .baseHertz = 0.0,
                .attackMs = 0,
                .decayMs = 40,
                .sustain = 0.0F,
                .releaseMs = 30,
                .maxHoldMs = 45,
                .filter = synth::FilterDesc{
                    .mode = synth::FilterMode::HighPass,
                    .cutoff = 1200.0,
                    .resonance = 1.0},
                .gain = 0.3F,
                .pan = 0.0F}};
    } // namespace

    const std::array<TrackPreset, kPresetCount> &trackPresets() noexcept
    {
        return kPresets;
    }

    std::string_view trackName(const std::size_t preset) noexcept
    {
        return kNames[preset];
    }

    std::optional<std::size_t> trackFor(
        const std::string_view name) noexcept
    {
        for (std::size_t preset = 0; preset < kPresetCount; ++preset)
        {
            if (kNames[preset] == name)
            {
                return preset;
            }
        }

        return std::nullopt;
    }

    VoiceDesc voiceFor(
        const TrackPreset &preset,
        const Controls &value,
        const FrameCount frames,
        const SampleRate rate,
        const std::uint64_t seed)
    {
        const auto note = value.get(kNote);

        const auto semitones =
            (note.has_value() ? note->approximate() : 0.0)
            + static_cast<double>(preset.transpose);

        // A note is a semitone above the preset's base.
        // Whatever o() and trans() added goes in with it.
        // Noise has no pitch, and its frequency is never read.
        const auto hertz = preset.baseHertz
            * std::pow(2.0, semitones / kSemitones);

        const auto hold = std::min(
            frames, framesForMs(preset.maxHoldMs, rate));

        // A rate with no depth said gets a gentle default.
        // So vib(6) alone is audible without a second call.
        const auto vibratoDepth =
            preset.vibratoHertz > 0.0 && preset.vibratoDepth == 0.0F
            ? kDefaultVibratoDepth
            : static_cast<double>(preset.vibratoDepth);

        // Semitones become a ratio here, where floats are at home.
        // The render path multiplies and never takes a power.
        const auto arpeggioRatio = preset.arpSemitones != 0
            ? std::pow(
                  2.0,
                  static_cast<double>(preset.arpSemitones) / kSemitones)
            : 1.0;

        return VoiceDesc{
            .shape = preset.shape,
            .frequency = hertz,
            .frequencySlide = preset.slide,
            .envelope = synth::Adsr{
                .attack = framesForMs(preset.attackMs, rate),
                .decay = framesForMs(preset.decayMs, rate),
                .sustain = preset.sustain,
                .release = framesForMs(preset.releaseMs, rate)},
            .hold = hold,
            .filter = preset.filter,
            .gain = preset.gain,
            .pan = preset.pan,
            .vibratoHertz = preset.vibratoHertz,
            .vibratoDepth = vibratoDepth,
            .arpeggioRatio = arpeggioRatio,
            .arpeggioPeriod = preset.arpSemitones != 0
                ? framesForMs(kArpeggioStepMs, rate)
                : 0,
            // Where the hit falls, rather than how long it is.
            // Two hits of one length are not one hit sounded twice.
            // And the same hit is the same hit on every run.
            .seed = seed};
    }

    namespace
    {
        // One of a note's extras, demoted alone if it is refused.
        // An extra differs from its note only in pitch and gain.
        // So a refusal here needs a note the synth already took.
        void soundExtra(
            synth::SynthMixer &mixer,
            const TrackPreset &sounded,
            const Controls &value,
            const FrameCount frames,
            const FrameIndex startFrame,
            const FrameIndex offset,
            const float gainScale)
        {
            auto voice = voiceFor(
                sounded, value, frames, mixer.format().rate,
                startFrame);
            voice.gain *= gainScale;

            try
            {
                mixer.trigger(
                    synth::TriggerRequest{
                        .voice = voice,
                        .startFrame = startFrame + offset});
            }
            // The no-match edge, for the reason soundNote() gives.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            catch (const synth::SynthError &)
            {
            }
            // GCOVR_EXCL_STOP
        }
    } // namespace

    bool soundNote(
        synth::SynthMixer &mixer,
        const TrackPreset &preset,
        const Controls &value,
        const FrameCount frames,
        const FrameIndex startFrame,
        const FrameIndex offset)
    {
        // A chain can promise what only the synth can refuse.
        // A cutoff past the device's Nyquist is the shipped example.
        // The chain never learns the rate, so it cannot ask first.
        // The note is demoted to silence rather than the run ended.
        try
        {
            mixer.trigger(
                synth::TriggerRequest{
                    // Seeded from where the note falls in the score.
                    // Not from where the device is.
                    // So a pause changes no hit's sound.
                    .voice = voiceFor(
                        preset,
                        value,
                        frames,
                        mixer.format().rate,
                        startFrame),
                    .startFrame = startFrame + offset});
        }
        // The excluded line is the no-match edge of the handler.
        // Only an exception that is not a SynthError would take it.
        // See docs/confirming-unreachable-branches.md.
        catch (const synth::SynthError &) // GCOVR_EXCL_LINE
        {
            return false;
        }

        // A second voice a fixed interval up, with every note.
        // An ordinary voice on the note's own terms, not an effect.
        // Worked out once, since the echo below sounds it again.
        const bool harmonised = preset.harmonySemitones != 0;
        auto above = preset;
        above.transpose = std::clamp(
            preset.transpose + preset.harmonySemitones, -120, 120);

        if (harmonised)
        {
            soundExtra(
                mixer, above, value, frames, startFrame, offset, 1.0F);
        }

        // One echo, quieter, a fixed way behind; nothing feeds back.
        // The harmony echoes too, since the echo is of what sounded.
        if (preset.delayMs > 0 && preset.delayMix > 0.0F)
        {
            const auto rate =
                static_cast<FrameIndex>(mixer.format().rate);
            const auto behind = startFrame
                + (static_cast<FrameIndex>(preset.delayMs) * rate)
                    / 1000;

            soundExtra(
                mixer, preset, value, frames, behind, offset,
                preset.delayMix);

            if (harmonised)
            {
                soundExtra(
                    mixer, above, value, frames, behind, offset,
                    preset.delayMix);
            }
        }

        return true;
    }

} // namespace antwika::music_editor
