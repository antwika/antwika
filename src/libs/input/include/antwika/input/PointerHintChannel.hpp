#pragma once

#include <optional>

#include "antwika/input/PointerHint.hpp"

namespace antwika::input
{

    /**
     * @brief The one channel in the system a replay does not reproduce:
     * where the pointer is, published once per tick, for something to
     * draw from.
     *
     * Everything else an application reacts to arrives as an
     * event::Event through ITickSource, is dispatched by
     * TickedEventDispatcher, and is therefore seen by TickEventRecorder
     * and written into a `--record` file. That is what makes a replay
     * reach the state its run reached, and it holds without exception
     * for anything a replay has to reproduce.
     *
     * A free-moving pointer is the one thing that cannot pay for it. A
     * window system reports motion at its own rate rather than the
     * application's -- several hundred a second into a run that ticks
     * twenty-five times -- and between two clicks none of it decides
     * anything. IdleMotionSource already keeps that motion out of the
     * recording for exactly that reason, at the cost of an application
     * being unable to draw anything that follows the pointer. This is
     * the channel that gives the drawing back without giving the
     * recording back with it.
     *
     * **The safety condition, and it is the whole of it: what is read
     * here may decide what is drawn, and nothing else.** A live run and
     * its replay do not agree on this value, deliberately -- a replay
     * holds none of the motion between clicks, so replaying publishes
     * only the positions the recorded events happen to carry. Fold a
     * hint into anything a replay has to reproduce and the two runs
     * diverge, silently, and the symptom shows up nowhere near the line
     * that caused it.
     *
     * **Why this is a value cell rather than an unrecorded event.** The
     * cheaper shape is a second event kind carrying a marker that
     * TickEventRecorder honours by skipping it. It was rejected. A
     * marked event still travels the dispatcher, so every ITickEventSink
     * an application owns is handed it, and the condition above stops
     * being a property of the wiring and becomes a rule each sink has to
     * remember. Here a sink cannot be handed a hint at all: a hint is
     * not an event, no dispatcher carries it, and reading one takes a
     * reference somebody passed in a main.cpp, in the open, next to the
     * pipeline that publishes it.
     *
     * That is the move game::UiOverlay and life::DragState already make
     * -- one shared fact written once per tick and read by whoever needs
     * it -- and the move ui::Scope makes for nesting: the use nobody
     * wants is harder to express than the use everybody does, rather
     * than merely written down somewhere.
     *
     * What it does not do is stop a sink that *was* handed the channel
     * from folding a hint into state. Nothing short of a language
     * feature would, so the accessor is named forRenderingOnly() and
     * every call site has to say so out loud.
     */
    class PointerHintChannel final
    {
    public:
        /**
         * @brief Replace what the channel holds.
         *
         * Called by PointerHintSource, once per tick that carried a
         * position. An application publishing its own hint is not
         * refused, and is also not a thing anything needs.
         *
         * @param hint Where the pointer now is.
         */
        void publish(PointerHint hint) noexcept;

        /**
         * @brief Read the latest hint, for deciding what to draw.
         *
         * Named for the only thing a caller may do with it, since a
         * replay does not reproduce this value and folding it into
         * anything a replay does reproduce makes a run and its replay
         * disagree.
         *
         * @return The last hint published, or nullopt while no event has
         * yet carried a position -- which is the state a run is in
         * before the pointer has been seen at all.
         */
        [[nodiscard]] std::optional<PointerHint>
        forRenderingOnly() const noexcept;

    private:
        std::optional<PointerHint> latest;
    };

} // namespace antwika::input
