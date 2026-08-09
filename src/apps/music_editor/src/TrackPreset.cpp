#include "antwika/music_editor/TrackPreset.hpp"

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

#include "ScoreText.hpp"

namespace antwika::music_editor
{

    namespace
    {
        constexpr double kDefaultVibratoDepth = 0.005;

        constexpr std::uint32_t kArpeggioStepMs = 40;
    }

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
    }

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

    FrameCount soundingFrames(
        const TrackPreset &preset,
        const FrameCount frames,
        const SampleRate rate)
    {
        const auto held =
            std::min(frames, framesForMs(preset.maxHoldMs, rate));

        return std::max<FrameCount>(
            held + framesForMs(preset.releaseMs, rate), 1);
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

        const auto hertz = preset.baseHertz
            * std::pow(2.0, semitones / kSemitones);

        const auto hold = std::min(
            frames, framesForMs(preset.maxHoldMs, rate));

        const auto vibratoDepth =
            preset.vibratoHertz > 0.0 && preset.vibratoDepth == 0.0F
            ? kDefaultVibratoDepth
            : static_cast<double>(preset.vibratoDepth);

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
            .seed = seed};
    }

    namespace
    {
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
            // GCOVR_EXCL_START
            catch (const synth::SynthError &)
            {
            }
            // GCOVR_EXCL_STOP
        }
    }

    bool soundNote(
        synth::SynthMixer &mixer,
        const TrackPreset &preset,
        const Controls &value,
        const FrameCount frames,
        const FrameIndex startFrame,
        const FrameIndex offset)
    {
        try
        {
            mixer.trigger(
                synth::TriggerRequest{
                    .voice = voiceFor(
                        preset,
                        value,
                        frames,
                        mixer.format().rate,
                        startFrame),
                    .startFrame = startFrame + offset});
        }
        catch (const synth::SynthError &) // GCOVR_EXCL_LINE
        {
            return false;
        }

        const bool harmonised = preset.harmonySemitones != 0;
        auto above = preset;
        above.transpose = std::clamp(
            preset.transpose + preset.harmonySemitones, -120, 120);

        if (harmonised)
        {
            soundExtra(
                mixer, above, value, frames, startFrame, offset, 1.0F);
        }

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

}
