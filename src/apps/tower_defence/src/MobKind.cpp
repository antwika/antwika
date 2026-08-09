#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    namespace
    {
    }

    MobProfile profileOf(const MobKind kind)
    {
        return kDefaultMobProfiles[static_cast<std::size_t>(kind)];
    }

}
