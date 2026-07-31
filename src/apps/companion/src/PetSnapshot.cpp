#include "antwika/companion/PetSnapshot.hpp"

namespace antwika::companion
{

    PetSnapshot snapshotOf(const Pet &pet)
    {
        return PetSnapshot{
            .state = pet.state(),
            .night = pet.night(),
            .hungry = pet.hungry(),
            .disturbed = pet.disturbed(),
            .hunger = pet.hunger(),
            .hungerMax = pet.settings().hungerMax,
            .happiness = pet.happiness(),
            .happinessMax = pet.settings().happinessMax,
            .ticks = pet.ticks()};
    }

} // namespace antwika::companion
