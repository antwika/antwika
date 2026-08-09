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
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    inline constexpr Size kDefaultSheetSize{
        .width = 512, .height = 768};

    struct EditorSummary final
    {
        std::uint64_t ticks = 0;
        std::uint64_t edits = 0;
        std::uint32_t saves = 0;
        std::uint32_t loads = 0;
        Size image{};

        std::vector<std::string> console;
    };

    using TickSinkFactory = std::function<std::unique_ptr<ITickEventSink>(
        const EditorState &, const UiOverlay &)>;

    struct EditorWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        const IInputEventCodec &codec;

        IAtlasStore &store;

        const Translator &translator;

        Size canvas;

        Size blank = kDefaultSheetSize;

        TileGrid tiles = {};

        std::optional<std::string> openPath = std::nullopt;

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        bool announceOpening = false;

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        TickSinkFactory extraSink = {};
    };

    EditorSummary bootstrap(const EditorWiring &config);

}
