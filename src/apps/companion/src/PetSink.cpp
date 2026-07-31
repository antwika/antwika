#include "antwika/companion/PetSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::companion
{

    PetSink::PetSink(Pet &pet) : pet(pet)
    {
    }

    void PetSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        pet.step();
    }

} // namespace antwika::companion
