#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/IClock.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/IFramePass.hpp"

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;
    using antwika::time::ISleeper;

    /**
     * @brief How long a tick lasts, and how many frames it is shown as.
     */
    struct FramePacing
    {
        /** @brief How long one tick takes, in total, however it is spent. */
        std::chrono::milliseconds tickInterval{0};

        /**
         * @brief How many frames are drawn per tick, at least one.
         *
         * One means the tick's own frame and nothing else, which is what
         * a plain frame limiter does.
         *
         * **A ceiling rather than a quota.** Each frame has a due time
         * measured from the top of the tick, and one whose time has
         * already gone by when it comes up is dropped rather than drawn
         * late -- so a machine that cannot manage this many draws inside
         * a tick shows fewer of them and the tick still lasts exactly
         * tickInterval. Raising it therefore costs nothing on a machine
         * that cannot use it, which is what makes a high value safe.
         */
        std::uint32_t framesPerTick = 1;
    };

    /**
     * @brief Paces a run, and draws the frames between two ticks.
     *
     * A decorator rather than a change to simulation::EngineLoop, which is
     * the one code path a live run and a replayed run share: putting a
     * render cadence in there would change its shape for every app to
     * give one of them a feature.
     *
     * The gap this fills is exactly the call to eventsFor(), which
     * EngineLoop makes at the top of every tick -- so the frames land
     * after the previous tick drew and before this tick's events are
     * read, which is precisely the interval a walker is part way
     * through a step.
     *
     * **It is a pure observer of the event stream.** eventsFor() returns
     * what the source it wraps returned, unchanged, so a recording is a
     * function of a stream this cannot touch, and attaching it is free
     * rather than merely cheap. That is the same standing
     * input::PointerHintSource has.
     *
     * Nothing a frame does can reach the simulation, for three separate
     * reasons: the events pass through untouched, IFramePass::draw() is
     * handed nothing it could write to, and no event is emitted, so this
     * sits harmlessly upstream of a recorder.
     *
     * The waiting is antwika::time::ISleeper's, which is what everything
     * in this project paces through, so a test asserts what was asked
     * for rather than spending the time.
     *
     * **Every wait is measured from the top of the tick rather than
     * from the last one, and that is what decides the frame rate.** A
     * pacer that slept one whole slice per frame added the sleeper's
     * own overshoot to every slice: a millisecond of scheduler
     * granularity per frame turns four frames over a forty-millisecond
     * tick into ten a tick's worth of *time*, and the run either draws
     * fewer frames a second than it asked for or -- worse -- takes
     * longer than a tick to tick. Both showed up as a frame counter
     * short of the rate the pacing named. Each frame is due at a fixed
     * offset instead, so an overshoot is absorbed by the next wait
     * rather than accumulated, the tick lands on tickInterval to
     * whatever the sleeper's resolution allows, and framesPerTick may
     * be raised until the machine rather than the pacing is the limit.
     */
    class FramePacedSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the source over what it wraps and how it waits.
         * @param inner Supplies each tick's events; must outlive this
         * object.
         * @param pass Draws the frames between ticks; must outlive this
         * object.
         * @param sleeper Does the waiting; must outlive this object.
         * @param clock Says how much of the tick has actually gone, so a
         * wait can be measured from the top of it; must outlive this
         * object.
         * @param pacing How long a tick lasts and how many frames show it.
         * @throws FramePacingError If pacing.framesPerTick is zero, since
         * a tick nothing is drawn on is a pacing nobody could have meant.
         */
        FramePacedSource(
            ITickEventSource &inner,
            IFramePass &pass,
            ISleeper &sleeper,
            const antwika::time::IClock &clock,
            FramePacing pacing);

        FramePacedSource(const FramePacedSource &) = delete;
        FramePacedSource(FramePacedSource &&) = delete;

        FramePacedSource &operator=(const FramePacedSource &) = delete;
        FramePacedSource &operator=(FramePacedSource &&) = delete;

        /**
         * @brief Draw this tick's share of frames, then get its events.
         *
         * The tick ends one whole interval after it began however many
         * frames were fitted into it, so changing the frame rate changes
         * how smooth a run looks and never how fast it goes.
         *
         * @param tick The tick to fetch events for.
         * @return Exactly what the wrapped source returned, unchanged.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        /**
         * @brief Wait until a point measured from the top of the tick.
         * @param started When the tick began.
         * @param elapsed How far into it to wait for.
         * @return Whether that point was still ahead when asked; false
         * means the moment has gone and the caller draws nothing for it.
         */
        [[nodiscard]] bool waitUntil(
            std::chrono::time_point<std::chrono::system_clock> started,
            std::chrono::milliseconds elapsed);

        ITickEventSource &inner;
        IFramePass &pass;
        ISleeper &sleeper;
        const antwika::time::IClock &clock;
        FramePacing pacing;
    };

} // namespace antwika::app
