#include "antwika/tower_defence/BattleSnapshot.hpp"

namespace antwika::tower_defence
{

    BattleSnapshot snapshotOf(const Battle &battle)
    {
        // The last initialiser carries the unwind branches.
        // They destroy the level copy and the two vectors.
        // Only an aggregate throwing part-built reaches them.
        // So only an allocation failure can take them.
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
        // The closing brace is the local snapshot's landing pad.
        // Nothing above it throws.
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
