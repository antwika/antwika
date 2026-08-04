#include "antwika/tower_defence/TowerDefenceSnapshotStore.hpp"

#include <antwika/console/SnapshotError.hpp>

#include "antwika/tower_defence/StateDump.hpp"

namespace antwika::tower_defence
{

    TowerDefenceSnapshotStore::TowerDefenceSnapshotStore(
        Campaign &campaign, std::uint64_t &best) noexcept
        : antwika::console::JsonSnapshotStore<StateDumpError>(
              {.magic = kStateDumpMagic,
               .version = kStateDumpVersion},
              "antwika tower defence state dump document",
              standardStateDumpMigrations),
          campaign(campaign),
          best(best)
    {
    }

    nlohmann::json TowerDefenceSnapshotStore::takeState(
        const std::string &)
    {
        // Named locals in assignment style, never nested temporaries.
        // A partly-built temporary needs conditional unwind cleanups.
        // Those are branches gcov counts and no input can take.
        StateDump dump;

        dump.campaign = campaign.remember();
        dump.bestScore = best;

        return stateDumpToJson(dump);
    }

    void TowerDefenceSnapshotStore::applyState(
        const std::string &, const nlohmann::json &state)
    {
        const StateDump dump = stateDumpFromJson(state);

        // The level regenerates from the seed and the dumped index.
        // What cannot be made to fit it is refused, not repaired.
        if (!campaign.restore(dump.campaign))
        {
            throw antwika::console::SnapshotError(
                "antwika::tower_defence: dump does not fit the level "
                "its seed regenerates");
        }

        best = dump.bestScore;
    }

} // namespace antwika::tower_defence
