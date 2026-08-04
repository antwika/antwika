#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IClipboard.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/sound/IDevice.hpp>
#include <antwika/synth/SynthMixer.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/music_editor/EditorScene.hpp"
#include "antwika/music_editor/EditorSink.hpp"
#include "antwika/music_editor/EditorState.hpp"
#include "antwika/music_editor/Playback.hpp"

namespace antwika::music_editor
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    /**
     * @brief What one run leaves behind, for a caller or a test.
     */
    struct EditorSummary
    {
        /** @brief How many voices were started in all. */
        std::uint64_t notes = 0;

        /** @brief How many ticks the musical clock advanced. */
        time::Tick played = 0;

        /** @brief How many lines were re-read while it ran. */
        std::size_t reparses = 0;

        /** @brief How many commands the last frame drew. */
        std::size_t commands = 0;
    };

    /**
     * @brief Builds one more tick sink over what bootstrap() owns.
     *
     * A factory rather than a sink, because whatever draws needs the
     * EditorSink holding the picture, and that does not exist before
     * bootstrap() has made one.
     */
    using TickSinkFactory =
        std::function<std::unique_ptr<ITickEventSink>(const EditorSink &)>;

    /**
     * @brief Everything a run needs that it does not own.
     */
    struct MusicEditorWiring
    {
        ILogger &logger;
        IEventSink &eventSink;
        ITickEventSource &inputSource;
        const IInputEventCodec &codec;
        const EditorScene &scene;

        /** @brief What sounds the notes; must outlive the run. */
        synth::SynthMixer &mixer;

        /** @brief What the mixer is pumped into; must outlive the run. */
        sound::IDevice &device;

        /** @brief Waits out queued audio; must outlive the run. */
        time::ISleeper &sleeper;

        /** @brief The two clocks and the two lookaheads. */
        PlaybackDesc playback;

        /** @brief The size the window was **asked** for. */
        Size canvas;

        /**
         * @brief Where a copy is mirrored to, or null.
         *
         * Absent on a replay, so replaying a session leaves this
         * machine's clipboard alone; see EditorSink's constructor.
         * An optional rather than a pointer, for the reason
         * GameWiring's own optionals give: absent means absent, and
         * there is no third state.
         */
        std::optional<std::reference_wrapper<input::IClipboard>>
            clipboard = std::nullopt;

        /**
         * @brief Where the menu's save writes and its load reads.
         */
        std::string scoresDirectory{"scores"};

        /**
         * @brief Whether a save reaches the disk.
         *
         * False on a replay, so replaying a session leaves this
         * machine's scores alone; see EditorSink's constructor.
         */
        bool writesScores = true;

        /**
         * @brief The scores there already are, sorted by name.
         *
         * Read by main() with listScores() before the loop, never
         * inside it; see EditorState::scores for why.
         */
        std::vector<std::string> scores{};

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder{};

        TickSinkFactory extraSink{};

        /** @brief A ceiling on the run, or none. */
        std::optional<time::Tick> maxTicks{};
    };

    /**
     * @brief Wire the editor up and run it until something stops it.
     * @param config Everything the run needs.
     * @return What the run left behind.
     */
    [[nodiscard]] EditorSummary bootstrap(const MusicEditorWiring &config);

} // namespace antwika::music_editor
