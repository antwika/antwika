#include "antwika/companion/CompanionSnapshotStore.hpp"

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>

#include "antwika/companion/CompanionMemory.hpp"
#include "antwika/companion/PetSave.hpp"
#include "antwika/companion/SaveFormatError.hpp"

namespace antwika::companion
{

    namespace
    {
        const antwika::console::SnapshotFormat &dumpFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const antwika::console::SnapshotFormat format(
                {.magic = kStateDumpMagic,
                 .version = kStateDumpVersion},
                "antwika companion state dump document",
                standardStateDumpMigrations); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        return antwika::replay::MigrationChain({}, kStateDumpVersion);
    }

    CompanionSnapshotStore::CompanionSnapshotStore(
        Pet &pet, Lineage &lineage) noexcept
        : pet(pet), lineage(lineage)
    {
    }

    void CompanionSnapshotStore::dump(
        const std::string &path,
        const std::vector<std::string> &console)
    {
        // The excluded lines are the envelope temporary's unwind arms.
        // Only a failed allocation inside them could take one.
        // See docs/confirming-unreachable-branches.md.
        dumpFormat().write(
            // GCOVR_EXCL_START
            antwika::console::Snapshot{
                .console = console,
                .state = companionMemoryToJson(CompanionMemory{
                    // GCOVR_EXCL_STOP
                    .pet = pet.remember(),
                    .lineage = lineage.remember()})},
            path);

        // The excluded line is the local snapshot's unwind destructor.
        // Nothing after its construction throws but the write itself.
    } // GCOVR_EXCL_LINE

    std::vector<std::string> CompanionSnapshotStore::load(
        const std::string &path)
    {
        auto snapshot = dumpFormat().read(path);

        try
        {
            const CompanionMemory memory =
                companionMemoryFromJson(snapshot.state);

            // Whole, and never field by field, as RestoreSink does.
            // There is no moment a half-restored companion exists.
            pet = Pet(pet.settings(), memory.pet);
            lineage = Lineage(memory.lineage);
        }
        // The state's own reader promises SaveFormatError.
        // What this seam promises is console::SnapshotError.
        // So it is rewrapped here, as game's store rewraps.
        catch (const SaveFormatError &failed) // GCOVR_EXCL_LINE
        {
            throw antwika::console::SnapshotError(failed.what());
        }

        return snapshot.console;
    }

} // namespace antwika::companion
