#pragma once

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/HapBuffer.hpp>
#include <antwika/pattern/Pattern.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/sequencer/FrameClock.hpp"
#include "antwika/sequencer/ISequencerSink.hpp"
#include "antwika/sequencer/TempoMap.hpp"

namespace antwika::sequencer
{

    using antwika::pattern::Cycle;
    using antwika::pattern::Pattern;

    /**
     * @brief How a sequencer is set up.
     */
    struct SequencerDesc
    {
        FrameClock clock;
        TempoMap tempo;

        /**
         * @brief How many ticks ahead of the current one to look.
         *
         * A whole number of ticks rather than a duration, so the window
         * a run queries is reproducible from the tick stream alone.
         * It has to cover at least a device's buffer, or a note will be
         * decided on after the frames it belonged to were rendered.
         */
        time::Tick lookahead = 1;
    };

    /**
     * @brief Turns a pattern into triggers on the frame timeline.
     *
     * **Where musical time meets frame time, and the only place the two
     * meet at all.**
     *
     * Each tick it advances a half-open window through *cycle* space,
     * queries the pattern once over it, and hands on every event that
     * begins.
     * The window is tracked in cycles rather than frames on purpose: a
     * frame maps back to a cycle through a division that need not land
     * exactly, and a boundary that drifted by one frame would either
     * sound an event twice or drop it.
     *
     * **It triggers on onsets, never on events.**
     * A pattern hands back a fragment for every event a window cut, and
     * sounding those would restart every held note at every window
     * boundary.
     */
    class Sequencer final
    {
    public:
        /**
         * @brief Construct a sequencer over its two clocks.
         * @param desc What to run against.
         * @throws SequencerError If the lookahead is no ticks at all.
         */
        explicit Sequencer(SequencerDesc desc);

        Sequencer(const Sequencer &) = delete;
        Sequencer(Sequencer &&) = delete;

        Sequencer &operator=(const Sequencer &) = delete;
        Sequencer &operator=(Sequencer &&) = delete;

        /**
         * @brief Decide what a tick's lookahead window makes sound.
         *
         * Idempotent in the only sense that matters: the window only
         * ever moves forward, so calling this for the same tick twice
         * sounds nothing the second time.
         *
         * @param tick The tick being stepped.
         * @param pattern What is playing.
         * @param out Where every event that begins is handed.
         * @throws SequencerError If the tick is too far along to place.
         * @throws antwika::pattern::PatternError If the exact
         * arithmetic will not fit.
         */
        void advance(
            time::Tick tick, const Pattern &pattern, ISequencerSink &out);

        /**
         * @brief Take up a run that is already in progress.
         *
         * Declares everything through that tick's window already asked
         * for, without querying anything and without sounding a note.
         *
         * **What it is for is a voice that was not there a moment
         * ago.** A sequencer built fresh has been asked nothing, so its
         * first advance() would query every cycle since the run began
         * and sound the lot at once; a caller adding a voice partway
         * through says here that the past is not its to play.
         *
         * Moving backwards is not expressible: a position already
         * passed leaves the window where it is.
         *
         * @param tick The tick to take the run up at.
         */
        void joinAt(time::Tick tick);

        /**
         * @brief Change tempo from a musical position onwards.
         *
         * The boundary may not fall inside anything already queried:
         * those events were placed on the old timeline and the frames
         * they landed on have been handed out, so moving them is not
         * expressible -- the same argument joinAt() makes about the
         * past.
         *
         * @param from Where the new tempo takes over; at or after
         * queriedThrough().
         * @param framesPerCycle How long one cycle lasts from there.
         * @throws SequencerError If the boundary is inside a window
         * already queried, not after every earlier change, or a cycle
         * would take no frames.
         */
        void retime(Cycle from, Rational framesPerCycle);

        /**
         * @brief Get how far the pattern has been asked about.
         * @return The first position no query has covered yet.
         */
        [[nodiscard]] Cycle queriedThrough() const noexcept;

    private:
        FrameClock clock;
        TempoMap tempo;
        time::Tick lookahead;

        Cycle asked;

        // Reserved once and cleared each tick, not built per tick.
        // So a run that never ends does not grow this one.
        // It is not the only allocation a query makes, though.
        // A combinator splitting a window builds a vector of its own.
        pattern::HapBuffer found;
    };

} // namespace antwika::sequencer
