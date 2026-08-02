#include "antwika/music_editor/TrackPreset.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include <antwika/pattern/Controls.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/synth/Adsr.hpp>
#include <antwika/synth/Filter.hpp>
#include <antwika/synth/VoiceDesc.hpp>
#include <antwika/synth/Waveshape.hpp>

namespace antwika::music_editor
{

    namespace
    {
        constexpr std::uint32_t kMillisecondsPerSecond = 1000;

        // Twelve semitones to the octave.
        // That is the only music theory this application contains.
        constexpr double kSemitonesPerOctave = 12.0;

        [[nodiscard]] FrameCount framesForMs(
            std::uint32_t milliseconds, SampleRate rate) noexcept
        {
            return static_cast<FrameCount>(milliseconds)
                * static_cast<FrameCount>(rate) / kMillisecondsPerSecond;
        }

        const std::array<TrackPreset, kTrackCount> kPresets{
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

    const std::array<TrackPreset, kTrackCount> &trackPresets() noexcept
    {
        return kPresets;
    }

    VoiceDesc voiceFor(
        const TrackPreset &preset,
        const Controls &value,
        const FrameCount frames,
        const SampleRate rate)
    {
        const auto note = value.get(kNote);

        const auto semitones =
            note.has_value() ? note->approximate() : 0.0;

        // A note is a semitone above the preset's base.
        // Noise has no pitch, and its frequency is never read.
        const auto hertz = preset.baseHertz
            * std::pow(2.0, semitones / kSemitonesPerOctave);

        const auto hold = std::min(
            frames, framesForMs(preset.maxHoldMs, rate));

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
            // Every noise hit differs from its neighbours.
            // The same hit is the same on every run.
            .seed = static_cast<std::uint64_t>(frames)};
    }

} // namespace antwika::music_editor
