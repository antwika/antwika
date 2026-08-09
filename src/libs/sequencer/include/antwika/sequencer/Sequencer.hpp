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

    struct SequencerDesc final
    {
        FrameClock clock;
        TempoMap tempo;

        time::Tick lookahead = 1;
    };

    class Sequencer final
    {
    public:
        explicit Sequencer(SequencerDesc desc);

        Sequencer(const Sequencer &) = delete;
        Sequencer(Sequencer &&) = delete;

        Sequencer &operator=(const Sequencer &) = delete;
        Sequencer &operator=(Sequencer &&) = delete;

        void advance(
            time::Tick tick, const Pattern &pattern, ISequencerSink &out);

        void joinAt(time::Tick tick);

        void retime(Cycle from, Rational framesPerCycle);

        [[nodiscard]] Cycle queriedThrough() const noexcept;

    private:
        FrameClock clock;
        TempoMap tempo;
        time::Tick lookahead;

        Cycle asked;

        pattern::HapBuffer found;
    };

}
