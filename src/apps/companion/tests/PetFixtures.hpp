#pragma once

#include <antwika/gfx/Size.hpp>

#include "antwika/companion/CompanionMemory.hpp"

#include "antwika/companion/Pet.hpp"

namespace antwika::companion::tests
{

    constexpr gfx::Size kCanvas{.width = 256, .height = 256};

    constexpr PetConfig kBrisk{
        .hungerPeriodTicks = 1,
        .starvePeriodTicks = 1000,
        .funDecayPeriodTicks = 1000,
        .fretPeriodTicks = 1000,
        .recoverPeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .drainHappyTicks = 1000,
        .drainContentTicks = 1000,
        .drainLowTicks = 1000,
        .drainMiserableTicks = 1000,
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .funMax = 8,
        .funStart = 8,
        .playFun = 2,
        .playHunger = 1,
        .playEnergy = 2,
        .energyBase = 20,
        .collapsePenalty = 10,
        .happinessMax = 6,
        .happinessStart = 4};

    constexpr PetConfig kUnhurried{
        .hungerPeriodTicks = 1000,
        .starvePeriodTicks = 1000,
        .funDecayPeriodTicks = 1000,
        .fretPeriodTicks = 1000,
        .recoverPeriodTicks = 1000,
        .restPeriodTicks = 1000,
        .drainHappyTicks = 1000,
        .drainContentTicks = 1000,
        .drainLowTicks = 1000,
        .drainMiserableTicks = 1000,
        .hungerMax = 8,
        .hungerThreshold = 2,
        .feedRelief = 2,
        .funMax = 8,
        .funStart = 8,
        .playEnergy = 2,
        .energyBase = 20,
        .collapsePenalty = 10,
        .happinessMax = 6,
        .happinessStart = 4};


    [[nodiscard]] inline CompanionMemory lived()
    {
        return CompanionMemory{
            .pet =
                PetMemory{
                    .ticks = 100,
                    .state = PetState::Awake,
                    .saying = Saying::Hello,
                    .sayingTicksLeft = 3,
                    .hunger = 5,
                    .fun = 4,
                    .happiness = 6,
                    .energy = 14,
                    .day = 3,
                    .meals = 7,
                    .plays = 5,
                    .disturbances = 2,
                    .pesters = 1,
                    .collapses = 0,
                    .woken = false},
            .lineage = LineageMemory{.generation = 2, .bestTicks = 50}};
    }

}
