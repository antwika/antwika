#pragma once

#include <optional>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/replay/IReplaySource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/input/IInputEventCodec.hpp"
#include "antwika/input/InputState.hpp"

namespace antwika::input
{

    using antwika::event::Event;
    using antwika::replay::IReplaySource;

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
     * **Not for an application that draws anything following a
     * free-moving pointer.** A hover highlight, a rubber band or a custom
     * cursor would update only when a button, a wheel or a key does,
     * because between those the movements are not in the tick stream to
     * draw from.
     *
     * **The name was reviewed and kept**, against a proposal to rename it
     * for that caveat (DiscardsHoverMotion or similar). It describes what
     * the decorator does to the stream, which is the level an
     * IReplaySource is named at, and it is true of every application. What
     * a name like that would describe is the consequence for an
     * application that draws a hover -- a property of that application,
     * not of this class, and one that neither antwika::life nor a headless
     * run has at all. The caveat belongs where an application chooses the
     * behaviour, which is InputPipelineOptions::thinIdleMotion, the field
     * a call site actually sets and where the same warning is written.
     */
    class IdleMotionSource final : public IReplaySource
    {
    public:
        /**
         * @brief Construct the decorator over what it wraps.
         * @param inner The source whose events pass through; must outlive
         * this object.
         * @param codec Decodes each event, to recognise movement and to
         * fold which buttons are down. Must outlive this object.
         */
        IdleMotionSource(IReplaySource &inner, const IInputEventCodec &codec);

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
        IReplaySource &inner;
        const IInputEventCodec &codec;

        // Folded from what arrives, never read from a device.
        // Only anyDown() is asked of it.
        // No movement it holds back can change that answer.
        InputState state;

        // The last movement held back, still unread by anything.
        std::optional<Event> latched;
    };

} // namespace antwika::input
