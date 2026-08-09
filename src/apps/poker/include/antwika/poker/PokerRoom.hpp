#pragma once

#include <optional>
#include <ostream>

#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/console/ConsolePicture.hpp>
#include <antwika/input/IInputEventCodec.hpp>

#include "antwika/poker/RoomConfig.hpp"
#include "antwika/poker/RoomSummary.hpp"
#include "antwika/poker/WindowSetup.hpp"

namespace antwika::poker
{

    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;
    using antwika::time::IClock;

    class PokerRoom final
    {
    public:
        explicit PokerRoom(IEngine &engine, ILogger &logger);

        PokerRoom(const PokerRoom &) = delete;
        PokerRoom(PokerRoom &&) = delete;

        PokerRoom &operator=(const PokerRoom &) = delete;
        PokerRoom &operator=(PokerRoom &&) = delete;

        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    struct RoomSetup final
    {
        IClock &clock;

        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        std::ostream &out;

        RoomConfig room = {};

        std::optional<
            std::reference_wrapper<const input::IInputEventCodec>>
            codec = std::nullopt;

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        std::optional<std::reference_wrapper<const WindowSetup>> window =
            std::nullopt;
    };

    RoomSummary bootstrap(const RoomSetup &setup);

    void printSummary(std::ostream &out, const RoomSummary &summary);

}
