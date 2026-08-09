#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>

#include <antwika/ecs/ISystem.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/engine/IEngine.hpp>
#include <antwika/event/IEventDispatcher.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/life/Board.hpp"
#include "antwika/life/DragState.hpp"
#include "antwika/life/Grid.hpp"
#include "antwika/life/PointerToggleSink.hpp"

namespace antwika::life
{

    using antwika::ecs::ISystem;
    using antwika::ecs::World;
    using antwika::engine::IEngine;
    using antwika::event::IEventDispatcher;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    [[nodiscard]] std::vector<std::reference_wrapper<ISystem>> observersFor(
        ISystem &renderer,
        ISystem &printer,
        ISystem &pacer,
        bool drawsNothing);

    void announceHowToStop(ILogger &logger, bool drawsNothing);

    using TickSinkFactory = std::function<std::unique_ptr<
        PointerToggleSink>(World &, const Grid &, DragState &)>;

    struct LifeSummary final
    {
        Board board;

        std::vector<std::string> console;

        [[nodiscard]] bool operator==(
            const LifeSummary &other) const = default;
    };

    class Life final
    {
    public:
        explicit Life(IEngine &engine, ILogger &logger);

        Life(const Life &) = delete;
        Life(Life &&) = delete;

        Life &operator=(const Life &) = delete;
        Life &operator=(Life &&) = delete;

        void run();

    private:
        IEngine &engine;
        ILogger &logger;
    };

    struct LifeWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        std::uint32_t width;

        std::uint32_t height;

        std::vector<std::reference_wrapper<ISystem>> observers = {};

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        TickSinkFactory extraSink = {};

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";
    };

    LifeSummary bootstrap(const LifeWiring &config);

}
