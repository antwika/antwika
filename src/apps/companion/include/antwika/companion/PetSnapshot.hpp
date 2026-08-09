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

    struct PetSnapshot final
    {
        PetState state = PetState::Awake;

        bool asleep = false;

        bool hungry = false;

        bool bored = false;

        bool tired = false;

        bool disturbed = false;

        Saying saying = Saying::None;

        std::uint32_t hunger = 0;
        std::uint32_t hungerMax = 0;
        std::uint32_t fun = 0;
        std::uint32_t funMax = 0;
        std::uint32_t happiness = 0;
        std::uint32_t happinessMax = 0;

        std::uint32_t energy = 0;

        std::uint32_t energyCeiling = 0;

        antwika::time::Tick ticks = 0;

        std::uint32_t day = 0;

        DayMood mood = DayMood::Ordinary;

        LifeStage stage = LifeStage::Egg;

        PetForm form = PetForm::Plain;

        LineageMemory lineage{};

        [[nodiscard]] bool operator==(const PetSnapshot &other) const
            = default;
    };

    [[nodiscard]] PetSnapshot snapshotOf(
        const Pet &pet, const Lineage &lineage);

}
