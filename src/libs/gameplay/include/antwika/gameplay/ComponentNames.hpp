#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/ecs/Entity.hpp>
#include <antwika/ecs/World.hpp>

#include "antwika/gameplay/SpawnContext.hpp"

namespace antwika::gameplay
{

    [[nodiscard]] std::span<const std::string_view> getComponentNames();

    [[nodiscard]] std::span<const std::string_view> getPlayerComponentNames();

    [[nodiscard]] std::span<const std::string_view> getCharacterComponentNames();

    [[nodiscard]] bool isComponentNamed(std::string_view name);

    void addComponentsNamed(
        ecs::World &world,
        ecs::Entity entity,
        const SpawnContext &spawnContext,
        std::span<const std::string> names);

    void claimModuleComponents(ecs::World &world);

}
