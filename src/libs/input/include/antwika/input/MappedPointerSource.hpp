#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/IPointerMapping.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Rewrites every position a device reported into the
     * coordinates the application lays itself out in.
     *
     * **The only decorator here that changes what an event says**, and
     * the one place in the stack that is allowed to: it sits upstream of
     * event::TickEventRecorder, so what lands in a recording is already
     * an application coordinate. That is the whole point. A file holding
     * device coordinates would only mean anything on a window of the
     * size it was recorded at, and replaying it anywhere else would
     * resolve every click somewhere else -- which is exactly the failure
     * docs/resizable-windows.md exists to prevent, arriving by a
     * different road.
     *
     * It changes the position and nothing else: no event is added,
     * dropped, reordered or moved to another tick, and an edge carrying
     * no position at all -- a key, a wheel notch -- passes through
     * untouched.
     *
     * **InputPipeline attaches it only when a device is being read**,
     * which is the one place the stack's usual symmetry is deliberately
     * broken. The thinning decorators are attached either way, because a
     * hand-authored file must replay the way the live run that would
     * have produced it ran. This is not a thinning: it is a change of
     * coordinate system applied to what a device said, and a file
     * already holds the result of applying it. Applying it a second time
     * on a replay would scale an already-scaled position.
     *
     * It is attached immediately outside LiveInputSource and inside
     * PointerHintSource, so what reaches the hint channel is in the same
     * coordinates as what reaches the tick stream -- otherwise a
     * placement ghost and the click that places it would disagree.
     */
    class MappedPointerSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it rewrites.
         * @param inner The source whose events are rewritten; must
         * outlive this object.
         * @param codec Decodes each event and encodes the rewritten one.
         * Must outlive this object.
         * @param mapping What a device position means on the
         * application's own surface. Must outlive this object.
         */
        MappedPointerSource(
            ITickEventSource &inner,
            const IInputEventCodec &codec,
            const IPointerMapping &mapping);

        MappedPointerSource(const MappedPointerSource &) = delete;
        MappedPointerSource(MappedPointerSource &&) = delete;

        MappedPointerSource &operator=(const MappedPointerSource &) = delete;
        MappedPointerSource &operator=(MappedPointerSource &&) = delete;

        /**
         * @brief Get a tick's events, with every position rewritten.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, in order, with the
         * position of each positional edge mapped.
         * @throws InputError If an input.* event carries a payload of
         * the wrong shape -- raised by the codec, since the wire format
         * is its to police.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;
        const IPointerMapping &mapping;
    };

} // namespace antwika::input
