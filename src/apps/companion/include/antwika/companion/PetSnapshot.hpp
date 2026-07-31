#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    /**
     * @brief Everything a frame needs, and nothing that can change it.
     *
     * The spectator's answer to Pet: what it looks like rather than what
     * it is made of, so the scene never learns what a period is or how
     * close the next one is.
     * Rendering is a write-only projection here as everywhere else, and
     * a value the scene cannot write is the structural way to say so.
     */
    struct PetSnapshot
    {
        /** @brief What it is doing. */
        PetState state = PetState::Awake;

        /** @brief Whether the clock says it is night. */
        bool night = false;

        /** @brief Whether it wants feeding. */
        bool hungry = false;

        /** @brief Whether tonight's rest has already been interrupted. */
        bool disturbed = false;

        std::uint32_t hunger = 0;
        std::uint32_t hungerMax = 0;
        std::uint32_t happiness = 0;
        std::uint32_t happinessMax = 0;

        /**
         * @brief How many ticks have been stepped.
         *
         * The one number the idle animation is resolved from, so the
         * picture is a function of the tick count and never of a clock.
         */
        antwika::time::Tick ticks = 0;

        /**
         * @brief Compare two snapshots field by field.
         * @param other The snapshot to compare against.
         * @return Whether every field matches.
         */
        [[nodiscard]] bool operator==(const PetSnapshot &other) const
            = default;
    };

    /**
     * @brief Take this tick's picture of a companion.
     * @param pet The companion to read.
     * @return The snapshot.
     */
    [[nodiscard]] PetSnapshot snapshotOf(const Pet &pet);

} // namespace antwika::companion
