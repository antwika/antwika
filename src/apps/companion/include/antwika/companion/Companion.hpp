#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/ITickSource.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::simulation::ITickSource;
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
        std::uint32_t hunger = 0;
        std::uint32_t happiness = 0;
        std::uint32_t meals = 0;
        std::uint32_t disturbances = 0;
        bool perished = false;
    };

    /**
     * @brief Builds one more tick sink over the state bootstrap() owns.
     *
     * A factory rather than a sink, because a sink that draws the
     * companion needs the Pet, and it does not exist before bootstrap()
     * has made one.
     * Ownership passes back, so the sink lives exactly as long as the
     * session it belongs to.
     */
    using TickSinkFactory =
        std::function<std::unique_ptr<ITickEventSink>(const Pet &)>;

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
        ITickSource &inputSource;

        /** @brief Decodes antwika::input's events. */
        const IInputEventCodec &codec;

        /** @brief Holds each tick back to the wall clock. */
        ISleeper &sleeper;

        /** @brief The numbers the companion is balanced with. */
        PetConfig pet = {};

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
     * @param config What the session is wired out of.
     * @return What the session ended on.
     * @throws CompanionError If the companion's numbers are ones no
     * session could be balanced on.
     */
    CompanionSummary bootstrap(const CompanionConfig &config);

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
