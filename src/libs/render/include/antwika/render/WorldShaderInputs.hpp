#pragma once

#include <cstddef>
#include <optional>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

namespace antwika::render
{

    struct WorldShaderInputs final
    {
        bool playing = false;

        bool lighting = true;

        bool sightOn = false;

        float ambient = 1.0F;

        gfx::Vec3 walkerPosition{};

        float fadeAbove = 0.0F;

        std::optional<std::size_t> carrying{};

        voxel::VoxelPosition hidingCornerPosition{};

        gfx::Vec3 sightPoint{};

        std::size_t sightSlot = 0;

        gfx::Vec3 upperSightPoint{};

        std::size_t upperSightSlot = 1;

        bool upperSightOn = true;

        gfx::Vec3 viewPosition{};

        gfx::Vec3 viewTargetPoint{};

        gfx::Color backdropColor{};
    };

}
