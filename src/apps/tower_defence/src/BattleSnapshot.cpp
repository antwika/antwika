#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    BattleSnapshot snapshotOf(const Campaign &campaign)
    {
        const Battle &battle = campaign.battle();

        BattleSnapshot snapshot{
            .level = battle.level(),
            .mobs = {},
            .towers = {},
            .towerRangeSquared =
                battle.settings().towerRangeSquared}; // GCOVR_EXCL_LINE

        snapshot.mobs.reserve(battle.mobs().size());
        for (const Mob &mob : battle.mobs())
        {
            snapshot.mobs.push_back(MobMarker{
                .cell = battle.level().path[mob.pathIndex],
                .kind = mob.kind});
        }

        snapshot.towers.reserve(battle.towers().size());
        for (const Tower &tower : battle.towers())
        {
            snapshot.towers.push_back(tower.cell);
        }

        return snapshot;
    } // GCOVR_EXCL_LINE

    std::uint32_t rangeRadius(const std::uint32_t rangeSquared)
    {
        std::uint32_t radius = 0;
        while ((radius + 1) * (radius + 1) <= rangeSquared)
        {
            ++radius;
        }
        return radius;
    }

}
