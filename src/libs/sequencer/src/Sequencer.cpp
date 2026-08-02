#include "antwika/sequencer/Sequencer.hpp"

#include <cstddef>
#include <utility>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/pattern/Pattern.hpp>
#include <antwika/pattern/Span.hpp>
#include <antwika/sound/Frames.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/sequencer/ISequencerSink.hpp"
#include "antwika/sequencer/SequencerError.hpp"

namespace antwika::sequencer
{

    namespace
    {
        // Enough for a busy bar without growing.
        // Nothing is lost if a window ever holds more.
        constexpr std::size_t kExpectedEvents = 64;
    } // namespace

    Sequencer::Sequencer(SequencerDesc desc)
        : clock(desc.clock),
          tempo(std::move(desc.tempo)),
          lookahead(desc.lookahead)
    {
        if (lookahead == 0)
        {
            throw SequencerError(
                "antwika::sequencer: looking no ticks ahead would decide "
                "a note after its frames had been rendered");
        }

        found.reserve(kExpectedEvents);
    }

    void Sequencer::advance(
        time::Tick tick, const Pattern &pattern, ISequencerSink &out)
    {
        const auto through = tempo.cycleAt(clock.frameAtTick(tick + lookahead));

        // The window only ever moves forward.
        // Asking again for a tick already covered sounds nothing.
        if (through <= asked)
        {
            return;
        }

        const pattern::Span window(asked, through);

        found.clear();
        pattern.query(window, found);

        for (const auto &hap : found.haps())
        {
            // The one line the whole library turns on.
            // A hap without an onset is a fragment.
            // Its event began earlier, in an earlier window.
            // Sounding it would restart a note already going.
            if (!hap.hasOnset())
            {
                continue;
            }

            const auto begins = tempo.framesAt(hap.whole->begin());
            const auto ends = tempo.framesAt(hap.whole->end());

            out.trigger(hap.value, begins, ends - begins);
        }

        asked = through;
    }

    void Sequencer::joinAt(const time::Tick tick)
    {
        const auto through =
            tempo.cycleAt(clock.frameAtTick(tick + lookahead));

        // The window only ever moves forward.
        // A position already passed is left where it is.
        if (through <= asked)
        {
            return;
        }

        asked = through;
    }

    Cycle Sequencer::queriedThrough() const noexcept
    {
        return asked;
    }

} // namespace antwika::sequencer
