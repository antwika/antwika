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
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/light/ActiveLight.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxel/VoxelCell.hpp>

namespace antwika::render
{

    class LightPasses final
    {
    public:
        void open(
            gfx::ViewportRenderer &viewportRenderer,
            const gfx::ShaderSource &shadowSource);

        void forget() noexcept;

        void hide(
            gfx::ViewportRenderer &viewportRenderer,
            std::set<voxel::VoxelCell> behindCells,
            voxel::VoxelCell aboutCell);

        [[nodiscard]] const std::set<voxel::VoxelCell> &hidden()
            const noexcept;

        void bakeLamps(
            gfx::ViewportRenderer &viewportRenderer,
            std::span<const std::unique_ptr<gfx::IMesh>> pileMeshes,
            const std::vector<light::ActiveLight> &lights);


        [[nodiscard]] std::span<const light::ActiveLight> lamps()
            const noexcept;

        [[nodiscard]] const gfx::ITexture *hiding() const noexcept;

        [[nodiscard]] const gfx::ITexture *lampShadows() const noexcept;


    private:
        std::unique_ptr<gfx::IShader> shadowShader;
        std::unique_ptr<gfx::IRenderTarget> lampShadowAtlasTarget;
        std::unique_ptr<gfx::ITexture> occlusionTexture;
        bool stale = true;
        std::vector<light::ActiveLight> bakedLights;
        std::set<voxel::VoxelCell> occludingCells;
        voxel::VoxelCell occlusionOriginCell{};
    };

}
