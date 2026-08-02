#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        // Indexed by the MobKind enumerator's own value.
        // A table rather than a switch.
        // A switch would leave a default arm nothing could reach.
        //
        // The four pull against each other rather than ranking.
        // A Runner is through a reach in a third of a Brute's ticks.
        // A Brute is worth more but blocks nothing while it plods.
        // A Shielded one is only worth building for once guns hit hard.
        constexpr std::array<MobProfile, kMobKindCount> kProfiles{
            MobProfile{
                .ticksPerCell = 2,
                .health = 6,
                .armour = 0,
                .reward = 10},
            MobProfile{
                .ticksPerCell = 1,
                .health = 4,
                .armour = 0,
                .reward = 14},
            MobProfile{
                .ticksPerCell = 3,
                .health = 18,
                .armour = 0,
                .reward = 24},
            MobProfile{
                .ticksPerCell = 2,
                .health = 8,
                .armour = 1,
                .reward = 30}};
    } // namespace

    MobProfile profileOf(const MobKind kind)
    {
        return kProfiles[static_cast<std::size_t>(kind)];
    }

} // namespace antwika::tower_defence
