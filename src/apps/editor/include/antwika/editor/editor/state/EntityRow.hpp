#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include <antwika/ecs/Entity.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/editor/state/EntityPick.hpp"

namespace antwika::editor
{

    struct EntityRow final
    {
        ecs::Entity entity = ecs::kNullEntity;

        std::optional<EntityKind> kind;

        std::size_t characterIndex = 0;

        bool player = false;

        /**
         * @brief The document cell the pick and the inspector work in.
         */
        voxel::VoxelPosition cellPosition{};

        /**
         * @brief Where the entity stands right now, for the camera.
         */
        std::optional<gfx::Vec3> aimPosition;

        std::string name;
    };

}
