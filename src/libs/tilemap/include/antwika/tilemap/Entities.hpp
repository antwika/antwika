#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include <antwika/geometry/Grid.hpp>

namespace antwika::tilemap
{

    struct Transition final
    {
        std::string id{};
        geometry::GridCell at{};
        std::string targetMap{};
        std::string targetEntry{};
        std::vector<std::string> requiredTags{};

        [[nodiscard]] bool operator==(const Transition &other) const
            = default;
    };

    struct BoatEmbark final
    {
        std::string id{};
        geometry::GridCell at{};

        [[nodiscard]] bool operator==(const BoatEmbark &other) const
            = default;
    };

    struct SpawnPoint final
    {
        std::string id{};
        geometry::GridCell at{};
        std::string enemy{};

        [[nodiscard]] bool operator==(const SpawnPoint &other) const
            = default;
    };

    struct Pickup final
    {
        std::string id{};
        geometry::GridCell at{};
        std::string item{};
        std::vector<std::string> grantedTags{};

        [[nodiscard]] bool operator==(const Pickup &other) const = default;
    };

    struct Npc final
    {
        std::string id{};
        geometry::GridCell at{};

        [[nodiscard]] bool operator==(const Npc &other) const = default;
    };

    struct TriggerVolume final
    {
        std::string id{};
        geometry::GridCell at{};
        std::uint32_t columns = 1;
        std::uint32_t rows = 1;
        std::string event{};
        std::vector<std::string> grantedTags{};

        [[nodiscard]] bool operator==(const TriggerVolume &other) const
            = default;
    };

    using Entity = std::variant<
        Transition,
        BoatEmbark,
        SpawnPoint,
        Pickup,
        Npc,
        TriggerVolume>;

}
