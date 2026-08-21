#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/map/MapFile.hpp>

namespace antwika::gameplay
{

    struct SpawnContext final
    {
        map::Placement placement{};

        std::uint32_t index = 0;
    };

    [[nodiscard]] std::span<const std::string_view> componentNames();

    [[nodiscard]] bool componentNamed(std::string_view name);

    void addComponentsNamed(
        ecs::World &world,
        ecs::Entity entity,
        const SpawnContext &spawnContext,
        std::span<const std::string> names);

    void claimNamedComponents(ecs::World &world);

}
