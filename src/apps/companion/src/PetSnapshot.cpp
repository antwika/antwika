#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    PetSnapshot snapshotOf(const Pet &pet, const Lineage &lineage)
    {
        return PetSnapshot{
            .state = pet.state(),
            .asleep = pet.night(),
            .hungry = pet.hungry(),
            .bored = pet.bored(),
            .tired = pet.tired(),
            .disturbed = pet.disturbed(),
            .saying = pet.saying(),
            .hunger = pet.hunger(),
            .hungerMax = pet.settings().hungerMax,
            .fun = pet.fun(),
            .funMax = pet.settings().funMax,
            .happiness = pet.happiness(),
            .happinessMax = pet.settings().happinessMax,
            .energy = pet.energy(),
            .energyCeiling = pet.energyCeiling(),
            .ticks = pet.ticks(),
            .day = pet.day(),
            .mood = pet.mood(),
            .stage = pet.stage(),
            .form = pet.form(),
            .lineage = lineage.remember()};
    }

}
