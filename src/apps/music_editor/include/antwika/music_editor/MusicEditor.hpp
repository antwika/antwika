#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
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

    struct EditorSummary final
    {
        std::uint64_t notes = 0;

        time::Tick played = 0;

        std::size_t reparses = 0;

        std::size_t commands = 0;

        std::vector<std::string> console{};
    };

    using TickSinkFactory =
        std::function<std::unique_ptr<ITickEventSink>(const EditorSink &)>;

    struct MusicEditorWiring final
    {
        ILogger &logger;
        IEventSink &eventSink;
        ITickEventSource &inputSource;
        const IInputEventCodec &codec;
        const EditorScene &scene;

        synth::SynthMixer &mixer;

        sound::IDevice &device;

        time::ISleeper &sleeper;

        PlaybackDesc playback;

        Size canvas;

        std::optional<std::reference_wrapper<input::IClipboard>>
            clipboard = std::nullopt;

        std::string scoresDirectory{"scores"};

        bool writesScores = true;

        std::vector<std::string> scores{};

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder{};

        TickSinkFactory extraSink{};

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = false;

        std::string stateDumpPath{"dump_state.json"};

        std::optional<time::Tick> maxTicks{};
    };

    [[nodiscard]] EditorSummary bootstrap(const MusicEditorWiring &config);

}
