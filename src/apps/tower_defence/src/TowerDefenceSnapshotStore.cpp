#include "antwika/tower_defence/TowerDefenceSnapshotStore.hpp"

#include <antwika/console/SnapshotError.hpp>
#include <antwika/console/SnapshotFormat.hpp>

#include "antwika/tower_defence/StateDump.hpp"

namespace antwika::tower_defence
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
                "antwika tower defence state dump document",
                standardStateDumpMigrations); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    TowerDefenceSnapshotStore::TowerDefenceSnapshotStore(
        Campaign &campaign, std::uint64_t &best) noexcept
        : campaign(campaign), best(best)
    {
    }

    void TowerDefenceSnapshotStore::dump(
        const std::string &path,
        const std::vector<std::string> &console)
    {
        // Named locals in assignment style, never nested temporaries.
        // A partly-built temporary needs conditional unwind cleanups.
        // Those are branches gcov counts and no input can take.
        StateDump dump;
        dump.campaign = campaign.remember();
        dump.bestScore = best;

        antwika::console::Snapshot snapshot;
        snapshot.console = console;
        snapshot.state = stateDumpToJson(dump);

        dumpFormat().write(snapshot, path);

        // The excluded line is the locals' unwind destructor.
        // Nothing after their construction throws but the write itself.
    } // GCOVR_EXCL_LINE

    std::vector<std::string> TowerDefenceSnapshotStore::load(
        const std::string &path)
    {
        auto snapshot = dumpFormat().read(path);

        StateDump dump;
        try
        {
            dump = stateDumpFromJson(snapshot.state);
        }
        // The state's own reader promises StateDumpError.
        // What this seam promises is console::SnapshotError.
        catch (const StateDumpError &failed) // GCOVR_EXCL_LINE
        {
            throw antwika::console::SnapshotError(failed.what());
        }

        // The level regenerates from the seed and the dumped index.
        // What cannot be made to fit it is refused, not repaired.
        if (!campaign.restore(dump.campaign))
        {
            throw antwika::console::SnapshotError(
                "antwika::tower_defence: dump does not fit the level "
                "its seed regenerates");
        }

        best = dump.bestScore;
        return snapshot.console;
    }

} // namespace antwika::tower_defence
