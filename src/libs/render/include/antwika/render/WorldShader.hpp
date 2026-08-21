#pragma once

#include <cstddef>
#include <span>
#include <memory>
#include <optional>
#include <vector>

#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/voxel/VoxelCell.hpp>

#include "antwika/render/WorldShaderInputs.hpp"

namespace antwika::render
{

    class WorldShader final
    {
    public:
        void open(
            gfx::ViewportRenderer &viewportRenderer,
            const gfx::ShaderSource &voxelSource);

        [[nodiscard]] gfx::IShader &program() const noexcept;

        void setLook(
            gfx::ViewportRenderer &viewportRenderer,
            const WorldShaderInputs &shaderInputs,
            const std::vector<light::ActiveLight> &lights,
            std::span<const light::ActiveLight> bakedLights) const;

    private:
        std::unique_ptr<gfx::IShader> voxelShader;
    };

}
