#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/sudoku/BoardOverlay.hpp"
#include "antwika/sudoku/Messages.hpp"
#include "antwika/sudoku/PuzzleState.hpp"
#include "antwika/sudoku/Solve.hpp"
#include "antwika/sudoku/Status.hpp"

namespace antwika::sudoku
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    struct SudokuSummary final
    {
        std::string grid;

        std::uint32_t filled = 0;

        Status status = Status::Playing;

        std::size_t commands = 0;

        std::vector<std::string> console;
    };

    using TickSinkFactory = std::function<std::unique_ptr<
        ITickEventSink>(const PuzzleState &, const BoardOverlay &)>;

    struct SudokuWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        const IInputEventCodec &codec;

        const Translator &translator;

        Size canvas;

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        TickSinkFactory extraSink = {};

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";
        std::uint64_t solveStepBudget = kSolveStepBudget;
    };

    SudokuSummary bootstrap(const SudokuWiring &config);

}
