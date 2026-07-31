#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    BattleSnapshot snapshotOf(const Battle &battle)
    {
        BattleSnapshot snapshot{
            .level = battle.level(),
            .mobs = {},
            .towers = {},
            .towerRangeSquared = battle.settings().towerRangeSquared,
            .score = battle.score(),
            .leaks = battle.leaks()};

        snapshot.mobs.reserve(battle.mobs().size());
        for (const Mob &mob : battle.mobs())
        {
            snapshot.mobs.push_back(battle.level().path[mob.pathIndex]);
        }

        snapshot.towers.reserve(battle.towers().size());
        for (const Tower &tower : battle.towers())
        {
            snapshot.towers.push_back(tower.cell);
        }

        return snapshot;
    }

    std::uint32_t rangeRadius(const std::uint32_t rangeSquared)
    {
        std::uint32_t radius = 0;
        while ((radius + 1) * (radius + 1) <= rangeSquared)
        {
            ++radius;
        }
        return radius;
    }

} // namespace antwika::tower_defence
