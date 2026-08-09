#include "antwika/companion/RestoreSink.hpp"

#include <sstream>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/Events.hpp"
#include "antwika/companion/PetSave.hpp"

namespace antwika::companion
{

    RestoreSink::RestoreSink(Pet &pet, Lineage &lineage)
        : pet(pet), lineage(lineage)
    {
    }

    void RestoreSink::handle(const TickEvent &event)
    {
        if (event.event.name != events::kRestore)
        {
            return;
        }

        std::istringstream document(event.event.payload);
        const CompanionMemory memory = readCompanionMemory(document);

        pet = Pet(pet.settings(), memory.pet);
        lineage = Lineage(memory.lineage);
    }

}
