#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/sequencer/FrameClock.hpp>
#include <antwika/sequencer/ISequencerSink.hpp>
#include <antwika/sequencer/Sequencer.hpp>
#include <antwika/sequencer/TempoMap.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/TrackPreset.hpp"

namespace antwika::music_editor
{

    using antwika::sound::FrameCount;
    using antwika::sound::FrameIndex;

    struct PlaybackDesc final
    {
        sequencer::FrameClock clock;

        sequencer::Rational framesPerCycle{};

        time::Tick lookahead = 3;

        time::Tick lead = 2;
    };

    struct ActiveNote final
    {
        std::size_t voice = 0;

        std::string chain{};

        std::size_t begin = 0;

        std::size_t length = 0;

        FrameIndex beginFrame = 0;

        FrameIndex endFrame = 0;
    };

    struct PlaybackMemory final
    {
        std::vector<sequencer::TempoMap::Segment> segments;

        sequencer::Rational retimed{};

        time::Tick played = 0;

        std::uint64_t counter = 0;

        FrameIndex queued = 0;

        FrameIndex pausedFrames = 0;

        std::size_t voiceCount = 0;

        [[nodiscard]] bool operator==(
            const PlaybackMemory &other) const = default;
    };

    class Playback final
    {
    public:
        Playback(
            const Score &score,
            synth::SynthMixer &mixer,
            sound::IDevice &device,
            time::ISleeper &sleeper,
            PlaybackDesc desc);

        Playback(const Playback &) = delete;
        Playback(Playback &&) = delete;

        Playback &operator=(const Playback &) = delete;
        Playback &operator=(Playback &&) = delete;

        void step(bool paused);

        [[nodiscard]] std::size_t sounding() const noexcept;

        [[nodiscard]] std::vector<DocumentSpan> highlights() const;

        void silence() noexcept;

        [[nodiscard]] std::uint64_t started() const noexcept;

        [[nodiscard]] std::size_t voices() const noexcept;

        [[nodiscard]] FrameIndex queuedFrames() const noexcept;

        [[nodiscard]] time::Tick playedTicks() const noexcept;

        [[nodiscard]] sequencer::Rational position() const;

        void setSpeed(sequencer::Rational speed);

        [[nodiscard]] PlaybackMemory remember() const;

        void restore(const PlaybackMemory &memory);

    private:
        class TrackVoices final : public sequencer::ISequencerSink
        {
        public:
            TrackVoices(
                synth::SynthMixer &mixer,
                std::uint64_t &counter,
                std::vector<ActiveNote> &notes);

            void trigger(
                const pattern::Controls &value,
                FrameIndex startFrame,
                FrameCount frames) override;

            FrameIndex offset = 0;

            std::size_t voiceIndex = 0;

            std::string_view chain{};

            TrackPreset preset{};

        private:
            synth::SynthMixer &mixer;
            std::uint64_t &counter;

            std::vector<ActiveNote> &notes;
        };

        struct Line final
        {
            std::unique_ptr<sequencer::Sequencer> sequencer;
            std::unique_ptr<TrackVoices> voices;
            time::Tick advanced = 0;
        };

        void grow(std::size_t count);

        void pump(bool paused);

        void pace();

        [[nodiscard]] FrameIndex soundedFrame() const;

        [[nodiscard]] FrameCount backlog() const;

        const Score &score;
        synth::SynthMixer &mixer;
        sound::IDevice &device;
        time::ISleeper &sleeper;

        PlaybackDesc shape;

        sequencer::TempoMap tempo;

        sequencer::Rational retimed{};

        std::vector<Line> perVoice;
        std::size_t voicesSounding = 0;

        std::vector<ActiveNote> active;

        FrameCount lead = 0;
        FrameIndex queued = 0;
        FrameIndex origin = 0;
        FrameIndex pausedFrames = 0;
        time::Tick played = 0;
        std::uint64_t counter = 0;

        std::chrono::milliseconds interval{0};
    };

}
