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
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/HighScore.hpp"
#include "antwika/tower_defence/IScoreStore.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief What one run leaves behind, for a caller or a test.
     */
    struct BattleSummary
    {
        std::uint64_t score = 0;
        std::uint32_t lives = 0;
        std::uint64_t ticks = 0;
        std::size_t towers = 0;
        std::size_t pathLength = 0;

        /** @brief The level being fought when the run ended, from one. */
        std::size_t level = 0;

        /** @brief How many waves of it had been released. */
        std::size_t wavesReleased = 0;

        /** @brief How far the campaign got. */
        CampaignPhase phase = CampaignPhase::Fighting;

        /** @brief The best any earlier run reached. */
        HighScore previousBest;

        /** @brief The record kept once this run was folded in. */
        HighScore best;
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the
     * campaign needs the Campaign and the ScoreOverlay, and neither
     * exists before bootstrap() has generated the first level.
     * Ownership passes back, so the sink lives exactly as long as the
     * run it belongs to.
     */
    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(
            const Campaign &, const ScoreOverlay &)>;

    /**
     * @brief Everything one run is wired out of.
     *
     * A struct with designated initialisers rather than a parameter
     * list, so a wrong argument is a compile error rather than a
     * silently different run.
     */
    struct TowerDefenceConfig
    {
        /** @brief Receives the run's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickEventSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /** @brief Words every label on the score bar. */
        const Translator &translator;

        /**
         * @brief The size everything is laid out and hit-tested against.
         *
         * The size the window was asked for, never the size one reports.
         */
        Size canvas;

        /** @brief Which campaign to play; the seed lives here. */
        CampaignConfig campaign = {};

        /**
         * @brief Where the best score is kept between runs.
         *
         * **Absent for a replay, and that is a rule rather than an
         * omission.** A record loaded from whatever happens to be on the
         * machine running a recording would put a number on the bar that
         * the recording never saw, and writing one back would let a
         * replayed run overwrite a record it did not earn.
         * storeIfLive() is where that decision is made, so no main() has
         * to remember it.
         *
         * Absent for a live run too is an ordinary state, and means a
         * run that starts from a best of zero and is not kept.
         */
        std::optional<std::reference_wrapper<IScoreStore>> scoreStore =
            std::nullopt;

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the run gives up rather than
         * going on forever.
         * Tests should always set it.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /** @brief Sink receiving every dispatched event, tick-stamped. */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /** @brief Factory for one more tick sink, e.g. the renderer. */
        TickSinkFactory extraSink = {};
    };

    /**
     * @brief Generate the first level, wire the sinks up and run the
     * loop.
     *
     * A live run and a replayed one are the same call: they differ only
     * in what inputSource was built from.
     *
     * **The record is read once, before the loop, and written once
     * after it** -- never from inside the tick path, where a file's
     * answer could differ between a live run and a replay of it. The
     * cost is stated rather than hidden: a run ended with Ctrl+C never
     * reaches the epilogue and so keeps nothing, exactly as a `--record`
     * run there writes no file.
     *
     * A record that will not load does not take the run with it: it is
     * logged, the run starts from a best of zero and goes on to write
     * over the file that would not read. A record that will not *write*
     * is thrown, since the run is already over and the one thing left to
     * say is that it was not kept.
     *
     * @param config What the run is wired out of.
     * @return What the run ended on.
     * @throws LevelError If no level could be generated.
     * @throws ScoreFormatError If there is a store and the record cannot
     * be written to it.
     */
    BattleSummary bootstrap(const TowerDefenceConfig &config);

    /**
     * @brief Offer a store to a live run and none to a replay.
     *
     * The whole of the replay rule, in a function a test can reach: an
     * application's main() is excluded from the coverage report and so
     * may hold no branch of its own, and "is this a replay?" is exactly
     * the branch that would otherwise live there.
     *
     * @param store The store a live run would use.
     * @param replayPath What `--replay` named, if anything.
     * @return The store, or nothing when a recording is being replayed.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<IScoreStore>>
    storeIfLive(
        IScoreStore &store,
        const std::optional<std::string> &replayPath);

    /**
     * @brief Word what a run came to, for the log.
     * @param summary What the run ended on.
     * @return One line, in English: a log line is a diagnostic and is
     * never translated.
     */
    [[nodiscard]] std::string summaryLine(const BattleSummary &summary);

} // namespace antwika::tower_defence
