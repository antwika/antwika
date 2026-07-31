#pragma once

#include <chrono>

#include <antwika/ecs/World.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/simulation/TickPacer.hpp>
#include <antwika/time/ISleeper.hpp>

namespace antwika::companion
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::log::ILogger;
    using antwika::time::ISleeper;

    /**
     * @brief Holds each tick back to the wall clock, so a run happens at
     * a speed somebody can watch.
     *
     * A run of this application has no end of its own -- it goes on
     * until the window is closed or a replay says to stop -- so an
     * unpaced one would spend a whole day of companion time in a
     * fraction of a second and burn a core doing it.
     *
     * **The waiting itself is simulation::TickPacer's**, which is the one
     * pacer this project has, and this is only the ITickEventSink shape
     * around it. That class is an ecs::ISystem because the two
     * applications that reach for it keep their state in a World and
     * register it as an observer; this one keeps its state in a plain
     * value, so the only thing standing between it and that class is the
     * World in the signature -- which TickPacer's own documentation says
     * it neither reads nor writes. An empty one is therefore the whole
     * adapter, and the alternative was a third copy of a class the
     * project has already deduplicated twice.
     *
     * Registered last, after whatever drew the frame, which makes the
     * order present-then-wait -- what a frame limiter is.
     * Waiting changes only how long a run takes and never what it
     * computes, so a replay of a paced run reaches the same state as an
     * unpaced one.
     */
    class PacingSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over how it waits, and for how long.
         * @param logger Handed to the empty World this owns; must
         * outlive this object.
         * @param sleeper Does the waiting; must outlive this object.
         * @param interval How long to wait, once per tick.
         */
        PacingSink(
            ILogger &logger,
            ISleeper &sleeper,
            std::chrono::milliseconds interval);

        PacingSink(const PacingSink &) = delete;
        PacingSink(PacingSink &&) = delete;

        PacingSink &operator=(const PacingSink &) = delete;
        PacingSink &operator=(PacingSink &&) = delete;

        /**
         * @brief Wait out this tick's interval, if this is a tick.
         * @param event The event to fold in; anything but engine.tick is
         * ignored.
         */
        void handle(const TickEvent &event) override;

    private:
        antwika::ecs::World world;
        antwika::simulation::TickPacer pacer;
    };

} // namespace antwika::companion
