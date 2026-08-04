#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        // Indexed by the MobKind enumerator's own value.
        // A table rather than a switch.
        // A switch would leave a default arm nothing could reach.
        //
    } // namespace

    MobProfile profileOf(const MobKind kind)
    {
        return kDefaultMobProfiles[static_cast<std::size_t>(kind)];
    }

} // namespace antwika::tower_defence
