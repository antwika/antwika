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

        // The store's own reader, over the payload instead of a file.
        // A recording naming an impossible companion is refused.
        // Exactly as a file naming one is.
        // There is one judge of that, and it is Pet's own constructor.
        std::istringstream document(event.event.payload);
        const CompanionMemory memory = readCompanionMemory(document);

        // Whole, and never field by field.
        // There is no moment at which a half-restored companion exists.
        // Which is the rule the restoring constructor was given.
        pet = Pet(pet.settings(), memory.pet);
        lineage = Lineage(memory.lineage);
    }

} // namespace antwika::companion
