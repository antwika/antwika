#include "antwika/render/LightPasses.hpp"

#include <utility>

#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/CubeFace.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshMaterial.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::render
{

    void LightPasses::open(
        gfx::ViewportRenderer &viewportRenderer,
        const gfx::ShaderSource &shadowSource)
    {
        shadowShader = viewportRenderer.createShader(shadowSource);
        lampShadowAtlasTarget = viewportRenderer.createRenderTarget(
            gfx::RenderTargetSpec{
                .size = light::shadowAtlasSize(), .depth = true});
    }

    void LightPasses::forget() noexcept
    {
        stale = true;
    }

    void LightPasses::hide(
        gfx::ViewportRenderer &viewportRenderer,
        voxel::Voxels behindVoxels,
        const voxel::VoxelPosition aboutPosition)
    {
        if (
            behindVoxels ==
                occludingVoxelsHeld && aboutPosition == occlusionOriginPosition
            && occlusionTexture)
        {
            return;
        }

        occludingVoxelsHeld = std::move(behindVoxels);
        occlusionOriginPosition = aboutPosition;

        const auto drawnOver = voxelmap::occlusionMask(
            occludingVoxelsHeld, voxelmap::occlusionMaskOrigin(aboutPosition));

        if (occlusionTexture)
        {
            viewportRenderer.updateTexture(*occlusionTexture, drawnOver);

            return;
        }

        occlusionTexture = viewportRenderer.createTexture(drawnOver);
    }

    const voxel::Voxels &LightPasses::hidden()
        const noexcept
    {
        return occludingVoxelsHeld;
    }

    void LightPasses::bakeLamps(
        gfx::ViewportRenderer &viewportRenderer,
        const std::span<const std::unique_ptr<gfx::IMesh>> pileMeshes,
        const std::vector<light::ActiveLight> &lights)
    {
        if (stale)
        {
            bakedLights.clear();
            stale = false;
        }

        if (pileMeshes.empty())
        {
            return;
        }

        const auto relit = light::dirtyShadowSlots(bakedLights, lights);

        for (const auto index : relit)
        {
            const auto standing = lights.at(index).position;

            for (const auto face : gfx::kEveryCubeFace)
            {
                const auto scope = viewportRenderer.targetScope(
                    *lampShadowAtlasTarget,
                    light::shadowFaceRect(index, face));

                viewportRenderer.clear(gfx::Color{});

                for (const auto &piece : pileMeshes)
                {
                    viewportRenderer.drawMesh(
                        *piece,
                        gfx::identityMatrix(),
                        light::shadowCamera(standing, face),
                        gfx::MeshMaterial{
                            .shader = shadowShader.get()});
                }
            }
        }

        bakedLights.resize(lights.size());

        for (const auto index : relit)
        {
            bakedLights.at(index) = lights.at(index);
        }
    }


    std::span<const light::ActiveLight> LightPasses::lamps()
        const noexcept
    {
        return bakedLights;
    }


    const gfx::ITexture *LightPasses::hiding() const noexcept
    {
        return occlusionTexture.get();
    }

    const gfx::ITexture *LightPasses::lampShadows() const noexcept
    {
        return lampShadowAtlasTarget->depth();
    }


}
