#pragma once

#include <cstddef>
#include <vector>

#include <antwika/pattern/Cycle.hpp>
#include <antwika/sound/Frames.hpp>

#include "antwika/sequencer/Rational.hpp"

namespace antwika::sequencer
{

    using antwika::pattern::Cycle;
    using antwika::sound::FrameIndex;

    class TempoMap final
    {
    public:
        struct Segment final
        {
            Cycle startCycle;

            FrameIndex startFrame = 0;

            Rational framesPerCycle;

            [[nodiscard]] bool operator==(
                const Segment &other) const = default;
        };

        explicit TempoMap(Rational framesPerCycle);

        void addSegment(Cycle startCycle, Rational framesPerCycle);

        [[nodiscard]] std::size_t segmentCount() const noexcept;

        [[nodiscard]] const std::vector<Segment> &
        segments() const noexcept;

        [[nodiscard]] FrameIndex framesAt(Cycle at) const;

        [[nodiscard]] Cycle cycleAt(FrameIndex frame) const;

    private:
        [[nodiscard]] const Segment &segmentFor(Cycle at) const noexcept;

        [[nodiscard]] const Segment &segmentFor(
            FrameIndex frame) const noexcept;

        std::vector<Segment> table;
    };

}
