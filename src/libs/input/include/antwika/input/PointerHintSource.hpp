#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/PointerHintChannel.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Publishes the pointer's latest position onto a
     * PointerHintChannel, and alters not one event doing it.
     *
     * The only decorator in the input stack that is a pure observer:
     * eventsFor() returns exactly what its inner source returned, in
     * order, unmodified. That is the property that makes attaching it
     * free. A recording is a function of the event stream, so a stream
     * this cannot alter is a recording this cannot alter, and an
     * application that adds a hint channel records byte for byte what it
     * recorded before -- which is asserted rather than asserted about.
     *
     * **Positions, not motion.** Every positional edge carries an
     * absolute position, so the last one of a tick is simply where the
     * pointer is; there is no folded state here to keep, and none to
     * overflow. A tick whose events carry no position -- only a scroll,
     * only a key, nothing at all -- publishes nothing, and the channel
     * goes on holding the last position anything reported, which is
     * still where the pointer is.
     *
     * **It belongs inside IdleMotionSource, and InputPipeline puts it
     * there.** The motion worth drawing is precisely the motion that
     * gate holds back, so a hint source downstream of the gate would
     * publish only what the gate let through and be exactly as blind as
     * the application it was added for. InputPipeline attaches it
     * immediately outside LiveInputSource, so no decorator can hide a
     * movement from it.
     *
     * The consequence is that the channel runs *ahead* of the event
     * stream: on the tick a gated movement arrives the channel already
     * has it, while the stream will not carry it until the next press,
     * wheel or key. That is the point of the channel, and it is also
     * exactly why a hint may never be mixed into anything a replay
     * reproduces -- see PointerHintChannel, which states the one rule
     * this whole mechanism rests on.
     *
     * Attached on a replay run as well as a live one, so the two
     * branches go on differing only in whether a device is read. What it
     * publishes there is whatever positions the recorded events carry,
     * which is fewer than the run had. A hover is not reproduced by a
     * replay, by construction, and that is a thing this design accepts
     * rather than a thing it hides.
     */
    class PointerHintSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the observer over what it watches.
         * @param inner The source whose events pass through untouched;
         * must outlive this object.
         * @param codec Decodes each event, to recognise the ones that
         * carry a position. Must outlive this object.
         * @param channel Where each tick's latest position is published.
         * Must outlive this object.
         */
        PointerHintSource(
            ITickEventSource &inner,
            const IInputEventCodec &codec,
            PointerHintChannel &channel);

        PointerHintSource(const PointerHintSource &) = delete;
        PointerHintSource(PointerHintSource &&) = delete;

        PointerHintSource &operator=(const PointerHintSource &) = delete;
        PointerHintSource &operator=(PointerHintSource &&) = delete;

        /**
         * @brief Get a tick's events, unchanged, having published the
         * last position any of them carried.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, exactly as it returned
         * them.
         * @throws InputError If an input.* event carries a payload of the
         * wrong shape -- raised by the codec, since the wire format is
         * its to police.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;
        PointerHintChannel &channel;
    };

} // namespace antwika::input
