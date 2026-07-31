#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickSource.hpp>
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/app/IFramePass.hpp"

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::simulation::ITickSource;
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
     */
    class FramePacedSource final : public ITickSource
    {
    public:
        /**
         * @brief Construct the source over what it wraps and how it waits.
         * @param inner Supplies each tick's events; must outlive this
         * object.
         * @param pass Draws the frames between ticks; must outlive this
         * object.
         * @param sleeper Does the waiting; must outlive this object.
         * @param pacing How long a tick lasts and how many frames show it.
         * @throws FramePacingError If pacing.framesPerTick is zero, since
         * a tick nothing is drawn on is a pacing nobody could have meant.
         */
        FramePacedSource(
            ITickSource &inner,
            IFramePass &pass,
            ISleeper &sleeper,
            FramePacing pacing);

        FramePacedSource(const FramePacedSource &) = delete;
        FramePacedSource(FramePacedSource &&) = delete;

        FramePacedSource &operator=(const FramePacedSource &) = delete;
        FramePacedSource &operator=(FramePacedSource &&) = delete;

        /**
         * @brief Draw this tick's share of frames, then get its events.
         *
         * The total waited is the whole tick interval however many
         * frames it was split into, so changing the frame rate changes
         * how smooth a run looks and never how fast it goes.
         *
         * @param tick The tick to fetch events for.
         * @return Exactly what the wrapped source returned, unchanged.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickSource &inner;
        IFramePass &pass;
        ISleeper &sleeper;
        FramePacing pacing;
    };

} // namespace antwika::app
