#pragma once

#include <cstdint>

#include <antwika/time/Tick.hpp>

#include "antwika/companion/DayMood.hpp"
#include "antwika/companion/LifeStage.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"
#include "antwika/companion/Saying.hpp"

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

        /**
         * @brief Whether it is asleep.
         *
         * There is no clock here to ask what time it is: a companion is
         * asleep because it was put to bed or because it dropped, and
         * the picture goes dark for that reason rather than for an hour.
         */
        bool asleep = false;

        /** @brief Whether it wants feeding. */
        bool hungry = false;

        /** @brief Whether it has run out of fun. */
        bool bored = false;

        /** @brief Whether it may be put to bed. */
        bool tired = false;

        /** @brief Whether today began with somebody waking it. */
        bool disturbed = false;

        /**
         * @brief What it is saying, or Saying::None for nothing.
         *
         * The line rather than the words, and no countdown: how long a
         * bubble has left is the simulation's business, and a scene
         * given it could start counting down itself.
         */
        Saying saying = Saying::None;

        std::uint32_t hunger = 0;
        std::uint32_t hungerMax = 0;
        std::uint32_t fun = 0;
        std::uint32_t funMax = 0;
        std::uint32_t happiness = 0;
        std::uint32_t happinessMax = 0;

        /** @brief What it lives on. */
        std::uint32_t energy = 0;

        /**
         * @brief The most it may hold, which shrinks as it collapses.
         *
         * A gauge whose own end moves, which is the whole of what a
         * collapse costs said as a picture rather than as a number.
         */
        std::uint32_t energyCeiling = 0;

        /**
         * @brief How many ticks have been stepped.
         *
         * The one number the idle animation is resolved from, so the
         * picture is a function of the tick count and never of a clock.
         */
        antwika::time::Tick ticks = 0;

        /** @brief Which day of its life it is on. */
        std::uint32_t day = 0;

        /** @brief What kind of day today is. */
        DayMood mood = DayMood::Ordinary;

        /** @brief How grown up it is. */
        LifeStage stage = LifeStage::Egg;

        /** @brief What it grew into. */
        PetForm form = PetForm::Plain;

        /** @brief What the file remembers across companions. */
        LineageMemory lineage{};

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
     * @param lineage The record the file keeps behind it.
     * @return The snapshot.
     */
    [[nodiscard]] PetSnapshot snapshotOf(
        const Pet &pet, const Lineage &lineage);

} // namespace antwika::companion
