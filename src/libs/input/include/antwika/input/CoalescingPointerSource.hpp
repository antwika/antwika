#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::simulation::ITickSource;

    /**
     * @brief Keeps only the last of each run of pointer movements within a
     * tick.
     *
     * A window system reports pointer motion far faster than an
     * application ticks, so a minute of dragging is tens of thousands of
     * input.pointer_move events in a recording, for a position that was
     * only ever read once per tick.
     *
     * Safe for determinism, because it sits in the source, upstream of
     * TickEventRecorder: what a recording contains is exactly what the run
     * consumed, so the recording still reproduces the run. This is the one
     * place a reduction like this may happen -- doing it after the recorder
     * would make the file disagree with the run, and doing it in a backend
     * would hide it behind the seam.
     *
     * **Lossless for a position, lossy for a path.** A drag delta summed
     * over the movements it drops equals the delta to the one it keeps, so
     * anything reading position() or delta() sees no difference. Anything
     * that cares which route the pointer took between two points -- a
     * freehand drawing tool, a gesture recogniser -- must not use this.
     *
     * Only consecutive movements collapse. A movement, then a click, then
     * a movement leaves both movements, because the click happened
     * somewhere specific and dropping the movement before it would move
     * it.
     */
    class CoalescingPointerSource final : public ITickSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must outlive
         * this object.
         */
        explicit CoalescingPointerSource(ITickSource &inner);

        CoalescingPointerSource(const CoalescingPointerSource &) = delete;
        CoalescingPointerSource(CoalescingPointerSource &&) = delete;

        CoalescingPointerSource &operator=(
            const CoalescingPointerSource &) = delete;
        CoalescingPointerSource &operator=(
            CoalescingPointerSource &&) = delete;

        /**
         * @brief Get a tick's events, with runs of movement thinned out.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, with all but the last of
         * each consecutive run of input.pointer_move removed.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickSource &inner;
    };

} // namespace antwika::input
