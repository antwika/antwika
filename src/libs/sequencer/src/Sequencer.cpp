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
        constexpr std::size_t kExpectedEvents = 64;
    }

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

        if (through <= asked)
        {
            return;
        }

        const pattern::Span window(asked, through);

        found.clear();
        pattern.query(window, found);

        for (const auto &hap : found.haps())
        {
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

        if (through <= asked)
        {
            return;
        }

        asked = through;
    }

    void Sequencer::retime(const Cycle from, const Rational framesPerCycle)
    {
        if (from < asked)
        {
            throw SequencerError(
                "antwika::sequencer: retiming a window already queried "
                "would move notes whose frames have been handed out");
        }

        tempo.addSegment(from, framesPerCycle);
    }

    Cycle Sequencer::queriedThrough() const noexcept
    {
        return asked;
    }

}
