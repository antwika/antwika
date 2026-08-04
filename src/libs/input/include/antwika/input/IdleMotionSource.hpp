#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/InputState.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    /**
     * @brief Holds back pointer movement that arrives while no button is
     * held, until something is in a position to read it.
     *
     * A movement with a button down is doing something -- panning a
     * camera, drawing on a board -- and passes through untouched. A
     * movement with every button up only updates where the application
     * believes the pointer to be, and nothing reads that belief until the
     * next press, release, scroll or key arrives. So the last such
     * movement is latched, superseding whatever was latched before it, and
     * released immediately ahead of the first event that is not one.
     *
     * That is what separates this from simply dropping the movement.
     * Folded position outlives the event that set it: input.pointer_scroll
     * carries no position of its own, so antwika::game::GridSink anchors a
     * zoom at the position the last movement reported. Dropping the idle
     * movements outright would anchor it wherever the pointer was last
     * recorded to be, half a window and a minute away.
     *
     * Everything downstream therefore reads the position it would have
     * read anyway. What it does not read is the several hundred
     * intermediate positions a window system reports between two ticks, at
     * about 140 bytes each in a recording.
     *
     * Safe for determinism for the reason CoalescingPointerSource is: it
     * sits upstream of TickEventRecorder, so a recording holds exactly
     * what the run consumed and the run and its replay see one stream.
     * The released movement carries the tick it was released on rather
     * than the one it arrived on, which is invisible downstream and is
     * what the recording says too. Attach it to the replay path as well as
     * the live one, so both run the same pipeline; running an already
     * gated stream through it again changes nothing.
     *
     * **Nothing following a free-moving pointer may be drawn from the
     * tick stream while this is attached.** A hover highlight, a rubber
     * band or a custom cursor read from there would update only when a
     * button, a wheel or a key does, because between those the movements
     * are not in that stream to draw from.
     *
     * That used to be the end of it, and it no longer is:
     * PointerHintChannel publishes the pointer's position on a channel
     * that is not the event stream, written by a PointerHintSource that
     * InputPipeline attaches inside this decorator and therefore sees
     * every movement this holds back. An application wanting both a thin
     * recording and a live hover names both settings, which is why
     * InputPipelineOptions describes them as a pair. What this class does
     * is unchanged -- the caveat above is still exactly true of the
     * stream, and reading a hint into anything a replay reproduces is
     * forbidden for reasons PointerHintChannel sets out at length.
     *
     * **The name was reviewed and kept**, against a proposal to rename it
     * for that caveat (DiscardsHoverMotion or similar). It describes what
     * the decorator does to the stream, which is the level an
     * ITickEventSource is named at, and it is true of every application. What
     * a name like that would describe is the consequence for an
     * application that draws a hover -- a property of that application,
     * not of this class, and one that neither antwika::life nor a headless
     * run has at all. The caveat belongs where an application chooses the
     * behaviour, which is InputPipelineOptions::thinIdleMotion, the field
     * a call site actually sets and where the same warning is written.
     */
    class IdleMotionSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must outlive
         * this object.
         * @param codec Decodes each event, to recognise movement and to
         * fold which buttons are down. Must outlive this object.
         */
        IdleMotionSource(
            ITickEventSource &inner, const IInputEventCodec &codec);

        IdleMotionSource(const IdleMotionSource &) = delete;
        IdleMotionSource(IdleMotionSource &&) = delete;

        IdleMotionSource &operator=(const IdleMotionSource &) = delete;
        IdleMotionSource &operator=(IdleMotionSource &&) = delete;

        /**
         * @brief Get a tick's events, with idle movement held back.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, less any movement that
         * arrived with no button held, and with the latest such movement
         * inserted ahead of the first event that is not one.
         * @throws InputError If an input.* event carries a payload of the
         * wrong shape -- raised by the codec, since the wire format is
         * its to police.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        const IInputEventCodec &codec;

        // Folded from what arrives, never read from a device.
        // Only anyDown() is asked of it.
        // No movement it holds back can change that answer.
        InputState state;

        // The last movement held back, still unread by anything.
        std::optional<Event> latched;
    };

} // namespace antwika::input
