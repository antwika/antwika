#include "antwika/companion/CompanionSnapshotStore.hpp"

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/PetSave.hpp"

namespace antwika::companion
{

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        return antwika::replay::MigrationChain({}, kStateDumpVersion);
    }

    CompanionSnapshotStore::CompanionSnapshotStore(
        Pet &pet, Lineage &lineage) noexcept
        : antwika::console::JsonSnapshotStore<SaveFormatError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika companion state dump document",
              standardStateDumpMigrations),
          pet(pet),
          lineage(lineage)
    {
    }

    nlohmann::json CompanionSnapshotStore::takeState(
        const std::string &)
    {
        CompanionMemory memory;

        memory.pet = pet.remember();
        memory.lineage = lineage.remember();

        return companionMemoryToJson(memory);
    }

    void CompanionSnapshotStore::applyState(
        const std::string &, const nlohmann::json &state)
    {
        const CompanionMemory memory = companionMemoryFromJson(state);

        // Whole, and never field by field, as RestoreSink does.
        // There is no moment a half-restored companion exists.
        pet = Pet(pet.settings(), memory.pet);
        lineage = Lineage(memory.lineage);
    }

} // namespace antwika::companion
