#include "antwika/ecs/ComponentKey.hpp"

#include <map>
#include <string>

#include "antwika/ecs/EcsError.hpp"

namespace antwika::ecs::detail
{

    void claimComponentKey(
        const ComponentKey key, const std::string_view name)
    {
        static std::map<ComponentKey, std::string> claimedNames;

        const auto [where, fresh] =
            claimedNames.try_emplace(key, std::string(name));

        if (!fresh && where->second != name)
        {
            throw EcsError(
                "ComponentKey: " + std::string(name) + " and "
                + where->second + " hash to one key");
        }
    }

}
