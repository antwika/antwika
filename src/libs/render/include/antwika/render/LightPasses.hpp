#pragma once

#include <memory>
#include <optional>
#include <set>
#include <span>
#include <vector>

#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/Voxels.hpp>

#include "antwika/render/MeshPiece.hpp"

namespace antwika::render
{

    class LightPasses final
    {
    public:
        void open(
            gfx::IRenderer &viewportRenderer,
            const gfx::ShaderSource &shadowSource);

        void forget() noexcept;

        void hide(
            gfx::IRenderer &viewportRenderer,
            voxel::Voxels behindVoxels,
            voxel::VoxelPosition aboutPosition);

        [[nodiscard]] const voxel::Voxels &getHiddenVoxels()
            const noexcept;

        void bakeLamps(
            gfx::IRenderer &viewportRenderer,
            std::span<const MeshPiece> pileMeshes,
            const std::vector<light::ActiveLight> &lights);


        [[nodiscard]] std::span<const light::ActiveLight> getLamps()
            const noexcept;

        [[nodiscard]] const gfx::ITexture *getHiding() const noexcept;

        [[nodiscard]] const gfx::ITexture *getLampShadows() const noexcept;


    private:
        std::unique_ptr<gfx::IShader> shadowShader;
        std::unique_ptr<gfx::IRenderTarget> lampShadowAtlasTarget;
        std::unique_ptr<gfx::ITexture> occlusionTexture;
        bool stale = true;
        std::vector<light::ActiveLight> bakedLights;
        voxel::Voxels occludingVoxelsHeld;
        voxel::VoxelPosition occlusionOriginPosition{};
    };

}
