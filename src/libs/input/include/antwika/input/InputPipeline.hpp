#pragma once

#include <functional>
#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/simulation/ITickSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/CoalescingPointerSource.hpp"
#include "antwika/input/IInputBackend.hpp"
#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/IdleMotionSource.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/LiveInputSource.hpp"
#include "antwika/input/PointerHintChannel.hpp"
#include "antwika/input/PointerHintSource.hpp"
#include "antwika/input/StopOnKeySource.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::simulation::ITickSource;

    /**
     * @brief What an application wants of its input, rather than how to
     * stack it.
     *
     * Every field is a policy an application is entitled to choose. Which
     * decorator each one attaches, and in what order, is InputPipeline's
     * business and not a call site's.
     */
    struct InputPipelineOptions
    {
        /**
         * @brief Whether to read a device at all.
         *
         * False for a `--replay` run, and the only field that describes
         * the run rather than the application: a replay already holds the
         * input it recorded, so reading a device too would make every
         * event arrive twice.
         */
        bool readsDevice = true;

        /**
         * @brief Whether to keep only the last of each run of pointer
         * movements inside a tick.
         *
         * **Not for an application that cares which route the pointer
         * took between two points.** antwika::life toggles every cell a
         * drag crosses, so thinning a run inside a tick would skip some.
         */
        bool coalescePointerMotion = false;

        /**
         * @brief Whether to hold back pointer movement that arrives while
         * no button is held.
         *
         * An application that draws something following a free-moving
         * pointer -- a hover highlight, a placement ghost, a custom
         * cursor -- must not draw it from the tick stream while this is
         * on, because between two clicks the movements are not in that
         * stream to draw from. It should name a pointerHint below and
         * draw from that instead, which is what the channel exists for
         * and what makes the two settings a pair rather than a choice.
         */
        bool thinIdleMotion = false;

        /**
         * @brief Where to publish the pointer's position for something
         * to draw from, if anywhere.
         *
         * Off unless an application names a channel, so nothing is
         * attached, nothing observes, and a run that names none records
         * byte for byte what it recorded before this existed.
         *
         * **What arrives there may decide what is drawn and nothing
         * else**, for the reason PointerHintChannel gives at length: it
         * is the one value in the system a replay does not reproduce, so
         * anything a replay *does* reproduce must not be a function of
         * it.
         */
        std::optional<std::reference_wrapper<PointerHintChannel>>
            pointerHint = std::nullopt;

        /**
         * @brief The key, if any, whose press ends the run.
         */
        std::optional<Key> stopOnKey = std::nullopt;
    };

    /**
     * @brief Assembles the input decorators an application asked for, in
     * the one order that is correct.
     *
     * The stack is, innermost first:
     *
     *     inner -> LiveInputSource -> PointerHintSource
     *           -> CoalescingPointerSource -> IdleMotionSource
     *           -> StopOnKeySource
     *
     * and every step of that order carries meaning that used to live in
     * a comment in a main.cpp.
     *
     * **LiveInputSource is innermost, so everything after it sees the
     * device.** Building a thinning decorator over the scripted source
     * and passing it *into* LiveInputSource compiles, runs, records and
     * replays consistently -- and thins nothing, because it only ever
     * sees the file. That was a real bug, fixed in 277c54b, and it is the
     * reason this class exists rather than a comment saying to be careful.
     *
     * **PointerHintSource comes straight after it, so no thinning
     * decorator can hide a movement from the hint channel.** What it
     * publishes is what the device reported, not what survived the
     * thinning -- which is the entire reason an application can gate its
     * recording and still draw a hover. It alters no event, so where it
     * sits changes nothing about the stream; only what it can see.
     *
     * **The thinning decorators are attached whether or not a device is
     * read.** A hand-authored file with several movements in one tick
     * must replay the way the live run that would have produced it ran,
     * so the two branches differ only in whether LiveInputSource is in
     * the stack. Re-applying either decorator to an already-thinned
     * stream changes nothing, which is what makes that safe.
     *
     * **StopOnKeySource is outermost, so it sees the key events
     * LiveInputSource produced**, and a stop it appends is ordinary
     * recorded input like anything else.
     *
     * All of it sits upstream of TickEventRecorder, which is the only
     * place a reduction may happen: doing it downstream would make the
     * recording disagree with the run that wrote it, and doing it in a
     * backend would hide it behind the seam.
     *
     * The hint channel does not qualify that rule, because it is not a
     * reduction of anything. Nothing leaves the stream on its account;
     * a second, unrecorded copy of a position is set beside the stream,
     * for drawing and for nothing else.
     */
    class InputPipeline final : public ITickSource
    {
    public:
        /**
         * @brief Assemble the pipeline over the source it seeds from.
         * @param inner Supplies each tick's recorded or scripted events;
         * must outlive this object.
         * @param backend Polled once per tick when options.readsDevice is
         * set, and untouched otherwise. Must outlive this object.
         * @param codec Encodes each edge, and decodes what the thinning
         * decorators have to recognise. Must outlive this object.
         * @param options What the application wants of its input.
         */
        InputPipeline(
            ITickSource &inner,
            IInputBackend &backend,
            const IInputEventCodec &codec,
            const InputPipelineOptions &options);

        InputPipeline(const InputPipeline &) = delete;
        InputPipeline(InputPipeline &&) = delete;

        InputPipeline &operator=(const InputPipeline &) = delete;
        InputPipeline &operator=(InputPipeline &&) = delete;

        /**
         * @brief Get a tick's events, as the assembled stack reports them.
         * @param tick The tick to fetch events for.
         * @return The inner source's events for that tick, plus whatever
         * a device reported, less whatever the attached decorators thin.
         * @throws InputError If an input.* event carries a payload of the
         * wrong shape -- raised by the codec, since the wire format is
         * its to police.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        // Each layer is engaged only when it was asked for.
        // Each holds a reference into the one engaged before it.
        // Neither copyable nor movable, so those references stay put.
        std::optional<LiveInputSource> live;
        std::optional<PointerHintSource> hinting;
        std::optional<CoalescingPointerSource> coalescing;
        std::optional<IdleMotionSource> idle;
        std::optional<StopOnKeySource> stopping;

        // The outermost layer engaged, which is what a tick asks.
        ITickSource *outermost;
    };

} // namespace antwika::input
