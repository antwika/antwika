#include "antwika/tower_defence/TowerDefenceSnapshotStore.hpp"

#include <antwika/console/SnapshotError.hpp>

#include "antwika/tower_defence/StateDump.hpp"

namespace antwika::tower_defence
{

    TowerDefenceSnapshotStore::TowerDefenceSnapshotStore(
        Campaign &campaign, std::uint64_t &best) noexcept
        : antwika::console::IJsonSnapshotStore<StateDumpError>(
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
        StateDump dump;

        dump.campaign = campaign.remember();
        dump.bestScore = best;

        return stateDumpToJson(dump);
    }

    void TowerDefenceSnapshotStore::applyState(
        const std::string &, const nlohmann::json &state)
    {
        const StateDump dump = stateDumpFromJson(state);

        if (!campaign.restore(dump.campaign))
        {
            throw antwika::console::SnapshotError(
                "antwika::tower_defence: dump does not fit the level "
                "its seed regenerates");
        }

        best = dump.bestScore;
    }

}
