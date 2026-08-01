#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

namespace antwika::companion
{

    using antwika::time::Tick;

    /**
     * @brief What the *file* remembers, as against what a companion
     * does.
     *
     * Kept apart from PetMemory deliberately, and not merely tidily: a
     * companion is destroyed and replaced whole by `Pet::revive()`, so
     * anything that has to outlive that cannot be one of its fields.
     * `Pet::remember()` stays the identity round trip it always was
     * because none of this is in it.
     */
    struct LineageMemory
    {
        /** @brief Which companion of this file this one is. */
        std::uint32_t generation = 1;

        /**
         * @brief The longest any companion of this file has lived.
         *
         * In ticks rather than in days, because a day is no longer a
         * fixed length -- bedtime is a verb now, so a day count would
         * reward short days rather than long lives. The picture divides
         * it back into days to say it out loud.
         */
        Tick bestTicks = 0;

        /**
         * @brief Compare two records field by field.
         * @param other The record to compare against.
         * @return Whether both fields match.
         */
        [[nodiscard]] bool operator==(const LineageMemory &other) const
            = default;
    };

    /**
     * @brief The record a file keeps across every companion in it.
     *
     * The whole of what makes losing one cost something: the number a
     * player was chasing is still on the screen after the companion
     * chasing it has gone.
     * Nothing is inherited but the record -- a new companion starts with
     * this build's own numbers, since inheriting anything else would be
     * one session wearing another's history.
     */
    class Lineage final
    {
    public:
        /**
         * @brief Construct the record, new or resumed.
         * @param memory What a file remembered, if anything.
         * @throws SaveFormatError If the generation is zero, which is a
         * companion that was never born rather than a numbering this
         * build could have written.
         */
        explicit Lineage(LineageMemory memory = {});

        /**
         * @brief Offer a companion's age to the record.
         *
         * Kept only when it beats what is there, so calling this twice
         * with the same companion is the same as calling it once -- and
         * the session's epilogue and a revival may both do so.
         *
         * @param ticks How long that companion lived.
         */
        void record(Tick ticks);

        /**
         * @brief Move on to the next companion.
         */
        void advance();

        /**
         * @brief Take the record as a value.
         * @return What a file keeps.
         */
        [[nodiscard]] LineageMemory remember() const;

        /**
         * @brief Get which companion of this file is current.
         * @return The generation, counted from one.
         */
        [[nodiscard]] std::uint32_t generation() const noexcept;

        /**
         * @brief Get the longest life this file has seen.
         * @return The ticks.
         */
        [[nodiscard]] Tick bestTicks() const noexcept;

    private:
        LineageMemory kept;
    };

} // namespace antwika::companion
