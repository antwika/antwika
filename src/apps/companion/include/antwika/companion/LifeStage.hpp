#pragma once

#include <cstdint>

namespace antwika::companion
{

    /**
     * @brief How grown up a companion is.
     *
     * Derived from the tick count rather than stored, which is the whole
     * reason this is a free function below and not a field: a companion
     * already counts its ticks and already saves them, so a stage kept
     * beside that count is a second copy of one fact that a restore
     * could put back out of step with the first.
     *
     * What a stage decides is how much energy the companion may hold,
     * and that is all -- growing up is capacity here, so the picture and
     * the gauge tell the same story with no number invented for it.
     */
    enum class LifeStage : std::uint8_t
    {
        /** @brief Newly hatched, and the frailest it will ever be. */
        Egg = 0,

        /** @brief Growing, with room for a longer day. */
        Child,

        /** @brief Longer still. */
        Teen,

        /** @brief As much room as it will ever have. */
        Adult,

        /**
         * @brief Past its best, and holding less than an adult did.
         *
         * The one stage that *lowers* the ceiling, which is why old age
         * can be what finally ends a companion that has collapsed often:
         * the room it lost to those collapses is not given back, and
         * this takes some of what is left.
         */
        Elder,
    };

    /**
     * @brief How well a companion was raised, as the counts it keeps.
     *
     * A plain value rather than the `Pet` itself, so `formFor()` below
     * can be exercised on a record no session had to produce.
     */
    struct CareRecord
    {
        std::uint32_t meals = 0;
        std::uint32_t plays = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;
        std::uint32_t collapses = 0;

        /**
         * @brief Compare two records field by field.
         * @param other The record to compare against.
         * @return Whether every count matches.
         */
        [[nodiscard]] bool operator==(const CareRecord &other) const
            = default;
    };

    /**
     * @brief What a companion grew into.
     *
     * Decided by the care record and nothing else, so what is standing
     * in the window at the end of a session is a report on how the
     * session went -- which is the only score this application keeps
     * that a player can see without reading a number.
     */
    enum class PetForm : std::uint8_t
    {
        /** @brief Raised without a single violation. */
        Bright = 0,

        /** @brief Raised well enough. */
        Plain,

        /** @brief Raised badly, and it shows. */
        Scruffy,
    };

    /**
     * @brief Weigh a care record into a form.
     *
     * A collapse counts for three, since it is the one violation that
     * takes something a companion never gets back.
     *
     * @param care What the companion has been through.
     * @return The form it wears.
     */
    [[nodiscard]] PetForm formFor(const CareRecord &care);

} // namespace antwika::companion
