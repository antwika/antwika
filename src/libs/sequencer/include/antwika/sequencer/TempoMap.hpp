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

    /**
     * @brief Where every cycle falls, in frames.
     *
     * A sorted table of segments, each holding where it starts in both
     * clocks and how many frames one cycle takes inside it.
     * A lookup is a search plus one exact multiply, so framesAt() and
     * cycleAt() are exact inverses at every segment boundary and
     * monotone everywhere between them.
     *
     * **No floating point anywhere**, which is the whole reason a note
     * lands where the score said it would rather than near it.
     */
    class TempoMap final
    {
    public:
        /**
         * @brief One stretch of the timeline at one tempo.
         *
         * Public so a caller can read the table back and rebuild an
         * equal map elsewhere: constructing at the first segment's
         * pace and replaying addSegment() for the rest reproduces
         * every startFrame, since each is derived from the segments
         * before it and nothing else.
         */
        struct Segment
        {
            /** @brief Where it starts, in cycles. */
            Cycle startCycle;

            /** @brief Where it starts, in frames. */
            FrameIndex startFrame = 0;

            /** @brief How many frames one cycle takes inside it. */
            Rational framesPerCycle;

            /**
             * @brief Compare two segments.
             * @param other The segment to compare against.
             * @return True when every field matches.
             */
            [[nodiscard]] bool operator==(
                const Segment &other) const = default;
        };

        /**
         * @brief Build a map at one tempo, from the very first frame.
         * @param framesPerCycle How long one cycle lasts.
         * @throws SequencerError If a cycle would take no frames at all.
         */
        explicit TempoMap(Rational framesPerCycle);

        /**
         * @brief Change tempo from a cycle onwards.
         * @param startCycle Where the new tempo takes over.
         * @param framesPerCycle How long one cycle lasts from there.
         * @throws SequencerError If the cycle is not after every segment
         * already added, or a cycle would take no frames.
         */
        void addSegment(Cycle startCycle, Rational framesPerCycle);

        /**
         * @brief Get how many segments this holds.
         * @return The count, never zero.
         */
        [[nodiscard]] std::size_t segmentCount() const noexcept;

        /**
         * @brief Get the whole table, first segment first.
         * @return The segments, never empty.
         */
        [[nodiscard]] const std::vector<Segment> &
        segments() const noexcept;

        /**
         * @brief Get the frame a musical position falls on.
         *
         * A position before the first segment is extrapolated from it,
         * so a pattern shifted early still lands somewhere sensible
         * rather than being refused.
         *
         * @param at The position.
         * @return The frame, floored.
         * @throws antwika::pattern::PatternError If the exact
         * arithmetic will not fit.
         */
        [[nodiscard]] FrameIndex framesAt(Cycle at) const;

        /**
         * @brief Get the musical position a frame falls on.
         * @param frame The frame.
         * @return The position, exactly.
         * @throws antwika::pattern::PatternError If the exact
         * arithmetic will not fit.
         */
        [[nodiscard]] Cycle cycleAt(FrameIndex frame) const;

    private:
        [[nodiscard]] const Segment &segmentFor(Cycle at) const noexcept;

        [[nodiscard]] const Segment &segmentFor(
            FrameIndex frame) const noexcept;

        std::vector<Segment> table;
    };

} // namespace antwika::sequencer
