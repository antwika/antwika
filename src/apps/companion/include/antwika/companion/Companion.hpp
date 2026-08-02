#pragma once

#include <chrono>
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
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/companion/IPetStore.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickEventSource;
    using antwika::time::ISleeper;

    /**
     * @brief How long one tick is on the wall clock.
     *
     * Derived from kTicksPerSecond rather than written out, so the one
     * constant that says how fast a companion lives also says how fast
     * it is drawn. Nothing inside the tick path reads this: it is what
     * PacingSink waits, and it reaches nothing else.
     */
    inline constexpr std::chrono::milliseconds kTickInterval{
        1000 / kTicksPerSecond};

    /**
     * @brief What one session leaves behind, for a caller or a test.
     */
    struct CompanionSummary
    {
        antwika::time::Tick ticks = 0;
        std::uint32_t day = 0;
        std::uint32_t hunger = 0;
        std::uint32_t fun = 0;
        std::uint32_t happiness = 0;
        std::uint32_t energy = 0;
        std::uint32_t energyCeiling = 0;
        std::uint32_t meals = 0;
        std::uint32_t plays = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;
        std::uint32_t collapses = 0;
        std::uint32_t generation = 1;
        antwika::time::Tick bestTicks = 0;
        bool perished = false;
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the
     * companion needs the Pet and the record behind it, and neither
     * exists before bootstrap() has made them.
     * Ownership passes back, so the sink lives exactly as long as the
     * session it belongs to.
     */
    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(const Pet &, const Lineage &)>;

    /**
     * @brief Everything one session is wired out of.
     *
     * A struct with designated initialisers rather than a parameter
     * list, so a wrong argument is a compile error rather than a
     * silently different session.
     */
    struct CompanionConfig
    {
        /** @brief Receives the session's diagnostics. */
        ILogger &logger;

        /** @brief Receives every dispatched event. */
        IEventSink &eventSink;

        /** @brief Supplies each tick's events, live or replayed. */
        ITickEventSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /** @brief Holds each tick back to the wall clock. */
        ISleeper &sleeper;

        /** @brief The numbers the companion is balanced with. */
        PetConfig pet = {};

        /**
         * @brief The size the props and the "new companion" button are
         * laid out and hit-tested against.
         *
         * The size the window was *asked* for, never the one it
         * reports, so a resized window cannot make a recorded press
         * land on a different answer.
         * Nothing by default, which is a session with no window: the
         * canvas is then too small for a grid, so there is nothing to
         * draw and nothing to press -- and every press is a prod.
         */
        Size canvas = {};

        /**
         * @brief Where the companion is kept between sessions.
         *
         * **Absent for a replay, and that is a rule rather than an
         * omission.** A replay reproduces the session it recorded, and
         * a companion loaded from whatever happens to be on the machine
         * running it is a different starting state and so a different
         * session -- the file would replay to one thing here and
         * another there, silently. Writing one is refused for the
         * mirror-image reason: a replayed session would overwrite the
         * live companion with one regenerated from a recording.
         * storeIfLive() is where that decision is made, so no main()
         * has to remember it.
         *
         * **A replay does not need one**, which is the whole point of
         * RestoreSource: what this store held is announced as a
         * companion.restore event ahead of the first tick, upstream of
         * the recorder, so a `--record` run writes the companion it was
         * played on into its own file. A session always begins on a
         * brand new companion and is restored by RestoreSink, live run
         * and replay alike, so the two reach the same animal by
         * construction rather than by happening to agree.
         *
         * Absent for a live run too is an ordinary state, and means a
         * session that starts new and is not kept.
         */
        std::optional<std::reference_wrapper<IPetStore>> store =
            std::nullopt;

        /** @brief How long to hold each tick for. */
        std::chrono::milliseconds tickInterval = kTickInterval;

        /**
         * @brief Safety cap on how many ticks to run.
         *
         * Reached without engine.stop, the session gives up rather than
         * going on forever.
         * Tests should always set it, since a companion has no end of
         * its own.
         */
        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        /** @brief Sink receiving every dispatched event, tick-stamped. */
        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        /** @brief Factory for one more tick sink, e.g. the renderer. */
        TickSinkFactory extraSink = {};
    };

    /**
     * @brief Wire the sinks up and run the loop.
     *
     * A live session and a replayed one are the same call: they differ
     * only in what inputSource was built from, which is what makes a
     * replay reproduce the companion by construction rather than by
     * convention.
     *
     * **When the companion is written out is once, after the loop has
     * finished** -- not every tick, which would be twenty writes a
     * second of a file nothing reads in between, and not on a timer,
     * which would be a clock inside a session that has none. The cost
     * is stated rather than hidden: a session killed with Ctrl+C never
     * reaches the epilogue and so keeps nothing. A `--record` run does
     * keep what it got to, because a recording is a log appended as the
     * run goes rather than a snapshot written after it. Closing the
     * window is the way to end one and keep the companion.
     *
     * A companion that will not load does not take the session with it:
     * it is logged and a new companion starts, and the session goes on
     * to write over the file that would not read. Keeping the unreadable
     * one would be kinder to whoever wants to look at it and would leave
     * the application never able to save again, which is worse.
     * A companion that will not *write* is thrown, since the session is
     * already over and the one thing left to say is that it was not
     * kept.
     *
     * @param config What the session is wired out of.
     * @return What the session ended on.
     * @throws CompanionError If the companion's numbers are ones no
     * session could be balanced on.
     * @throws SaveFormatError If there is a store and the companion
     * cannot be written to it.
     */
    CompanionSummary bootstrap(const CompanionConfig &config);

    /**
     * @brief Offer a store to a live session and none to a replay.
     *
     * The whole of the replay rule, in a function a test can reach: an
     * application's main() is excluded from the coverage report and so
     * may hold no branch of its own, and "is this a replay?" is exactly
     * the branch that would otherwise live there.
     *
     * @param store Where a live session keeps its companion.
     * @param replayPath What `--replay` named, if anything.
     * @return The store, or nothing when a replay is being run.
     */
    [[nodiscard]] std::optional<std::reference_wrapper<IPetStore>>
        storeIfLive(
            IPetStore &store,
            const std::optional<std::string> &replayPath);

    /**
     * @brief Say how to stop a session nobody can close a window on.
     *
     * A session has no end of its own: it goes on until the window is
     * closed, or until a replay says to stop. A headless build reports
     * neither.
     *
     * @param logger Where the notice is written.
     * @param drawsNothing Whether the backend shows anything at all.
     */
    void announceHowToStop(ILogger &logger, bool drawsNothing);

    /**
     * @brief Word a finished session as one line of log.
     *
     * A function rather than a statement in main(), because an
     * application's main.cpp is excluded from the coverage report and
     * has to earn it by holding no branch -- and "it perished" against
     * "it is still with us" is a branch.
     *
     * @param summary What the session ended on.
     * @return The line to log.
     */
    [[nodiscard]] std::string summaryLine(const CompanionSummary &summary);

} // namespace antwika::companion
