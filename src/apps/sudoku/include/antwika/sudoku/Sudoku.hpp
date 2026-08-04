#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
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
    using antwika::simulation::ITickEventSource;

    /**
     * @brief What one session leaves behind, for a caller or a test.
     */
    struct SudokuSummary
    {
        /**
         * @brief The grid the session ended on, in its flat form.
         *
         * The one thing worth comparing between a live run and its
         * replay, and the reason it is here rather than left inside the
         * state the run destroys.
         */
        std::string grid;

        /** @brief How many of the 81 squares hold a digit. */
        std::uint32_t filled = 0;

        /** @brief What the session last said. */
        Status status = Status::Playing;

        /** @brief How many commands the last frame drew. */
        std::size_t commands = 0;
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the
     * session needs the BoardOverlay, and that does not exist before
     * bootstrap() has made one.
     * Ownership passes back, so the sink lives exactly as long as the
     * run it belongs to.
     */
    using TickSinkFactory = std::function<std::unique_ptr<
        ITickEventSink>(const PuzzleState &, const BoardOverlay &)>;

    /**
     * @brief Everything one session is wired out of.
     *
     * A struct with designated initialisers rather than a parameter
     * list, so a wrong argument is a compile error rather than a
     * silently different run.
     */
    struct SudokuWiring
    {
        /** @brief Receives the run's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /**
         * @brief Supplies each tick's events, live or replayed.
         *
         * The puzzle arrives through here too, as this application's
         * own sudoku.new_puzzle -- see PuzzleSource.
         */
        ITickEventSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /**
         * @brief Words every label the picture declares.
         *
         * The bar is measured from what this says and the grid sits
         * under whatever height it comes to, so the locale has to be
         * the same on the recording machine and the replaying one --
         * which is why main() fixes it and reads one from nowhere else.
         */
        const Translator &translator;

        /**
         * @brief The size everything is laid out and hit-tested
         * against.
         *
         * The size the window was asked for, never the size one
         * reports.
         */
        Size canvas;

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run throws rather than going
         * on forever; a session itself ends through TickLimitSource,
         * which is an ordinary stop.
         * Tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /** @brief Sink receiving every dispatched event, tick-stamped. */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /** @brief Factory for one more tick sink, e.g. the renderer. */
        TickSinkFactory extraSink = {};
        /**
         * @brief How much search one press of Solve may spend.
         *
         * Read off config.json by main() -- see ConfigFile.hpp -- and
         * defaulted to the shipped budget, so a caller that says
         * nothing solves exactly as it always did.
         */
        std::uint64_t solveStepBudget = kSolveStepBudget;
    };

    /**
     * @brief Wire the sinks up and run the loop.
     *
     * A live run and a replayed one are the same call: they differ only
     * in what inputSource was built from.
     *
     * @param config What the session is wired out of.
     * @return What the session ended on.
     * @throws antwika::simulation::EngineLoopError If maxTicks is
     * reached without an engine.stop.
     * @throws BoardFormatError If an event carries a payload that is
     * not the shape its schema describes.
     */
    SudokuSummary bootstrap(const SudokuWiring &config);

} // namespace antwika::sudoku
