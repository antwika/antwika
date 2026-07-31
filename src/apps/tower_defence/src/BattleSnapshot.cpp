#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    BattleSnapshot snapshotOf(const Battle &battle)
    {
        // The last initialiser carries the unwind branches that destroy
        // the level copy and the two vectors if the aggregate throws
        // part-built.
        // Only an allocation failure can take them.
        BattleSnapshot snapshot{
            .level = battle.level(),
            .mobs = {},
            .towers = {},
            .towerRangeSquared = battle.settings().towerRangeSquared,
            .score = battle.score(),
            .leaks = battle.leaks()}; // GCOVR_EXCL_LINE

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
        // The closing brace is the unwind landing pad destroying the
        // local snapshot; nothing above it throws.
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

} // namespace antwika::tower_defence
