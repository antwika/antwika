#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/pattern/Controls.hpp>
#include <antwika/pattern/ParamId.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/WaveFormat.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/synth/VoiceDesc.hpp>

namespace antwika::music_editor
{

    using antwika::pattern::Controls;
    using antwika::pattern::ParamId;
    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;
    using antwika::sound::SampleRate;
    using antwika::synth::VoiceDesc;

    inline constexpr std::size_t kPresetCount = 4;

    inline constexpr ParamId kNote{1};

    inline constexpr ParamId kSpanBegin{2};

    inline constexpr ParamId kSpanLength{3};

    struct TrackPreset final
    {
        synth::Waveshape shape = synth::Waveshape::Sine;

        double baseHertz = 220.0;

        double slide = 0.0;

        std::int32_t transpose = 0;

        std::uint32_t attackMs = 0;
        std::uint32_t decayMs = 0;
        float sustain = 1.0F;
        std::uint32_t releaseMs = 60;

        std::uint32_t maxHoldMs = 2000;

        synth::FilterDesc filter{};

        double vibratoHertz = 0.0;

        float vibratoDepth = 0.0F;

        std::int32_t arpSemitones = 0;

        std::uint32_t delayMs = 0;

        float delayMix = 0.5F;

        std::int32_t harmonySemitones = 0;

        float gain = 0.4F;
        float pan = 0.0F;

        [[nodiscard]] bool operator==(const TrackPreset &other) const
            = default;
    };

    [[nodiscard]] const std::array<TrackPreset, kPresetCount> &
        trackPresets() noexcept;

    [[nodiscard]] std::string_view trackName(std::size_t preset) noexcept;

    [[nodiscard]] std::optional<std::size_t> trackFor(
        std::string_view name) noexcept;

    [[nodiscard]] VoiceDesc voiceFor(
        const TrackPreset &preset,
        const Controls &value,
        FrameCount frames,
        SampleRate rate,
        std::uint64_t seed);

    /**
     * @brief Counts the frames a note stays audible for.
     *
     * @param preset The voice the note sounds through.
     * @param frames The frames the note's slot lasts.
     * @param rate The frames a second the mixer runs at.
     * @return The frames from the note's onset to the end of its
     *         release, at least one.
     */
    [[nodiscard]] FrameCount soundingFrames(
        const TrackPreset &preset, FrameCount frames, SampleRate rate);

    bool soundNote(
        synth::SynthMixer &mixer,
        const TrackPreset &preset,
        const Controls &value,
        FrameCount frames,
        FrameIndex startFrame,
        FrameIndex offset);

}
