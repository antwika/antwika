#include "antwika/render/WorldShader.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <utility>

#include <antwika/gfx/CubeFace.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::render
{

    namespace
    {
        [[nodiscard]] gfx::Vec3 sightOrigin(
            const std::size_t slot,
            const gfx::Vec3 livePoint,
            const std::vector<light::ActiveLight> &lights,
            const std::span<const light::ActiveLight> bakedLights)
        {
            return slot < lights.size() && lights.at(slot).castsShadows
                           && slot < bakedLights.size()
                       ? bakedLights[slot].position
                       : livePoint;
        }

        constexpr std::array<std::string_view,
            light::kMaxLamps> kLampAtNames{
            "lampAt[0]",
            "lampAt[1]",
            "lampAt[2]",
            "lampAt[3]",
            "lampAt[4]",
            "lampAt[5]",
            "lampAt[6]",
            "lampAt[7]"};

        constexpr std::array<std::string_view, light::kMaxLamps>
            kLampReachNames{
                "lampReach[0]",
                "lampReach[1]",
                "lampReach[2]",
                "lampReach[3]",
                "lampReach[4]",
                "lampReach[5]",
                "lampReach[6]",
                "lampReach[7]"};

        constexpr std::array<std::string_view, light::kMaxLamps>
            kLampBrightnessNames{
                "lampBrightness[0]",
                "lampBrightness[1]",
                "lampBrightness[2]",
                "lampBrightness[3]",
                "lampBrightness[4]",
                "lampBrightness[5]",
                "lampBrightness[6]",
                "lampBrightness[7]"};

        constexpr std::array<std::string_view, light::kMaxLamps>
            kLampTintNames{
                "lampTint[0]",
                "lampTint[1]",
                "lampTint[2]",
                "lampTint[3]",
                "lampTint[4]",
                "lampTint[5]",
                "lampTint[6]",
                "lampTint[7]"};

        constexpr std::array<std::string_view, light::kMaxLamps>
            kLampShadowNames{
                "lampShadows[0]",
                "lampShadows[1]",
                "lampShadows[2]",
                "lampShadows[3]",
                "lampShadows[4]",
                "lampShadows[5]",
                "lampShadows[6]",
                "lampShadows[7]"};
    }

    void WorldShader::open(
        gfx::IRenderer &viewportRenderer,
        const gfx::ShaderSource &voxelSource)
    {
        voxelShader = viewportRenderer.createShader(voxelSource);

        viewportRenderer.setShaderNumber(
            *voxelShader, "levelFade", light::kLevelFade);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "hidingSpan",
            static_cast<float>(voxelmap::kOcclusionMaskWidth));
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "hidingLevels",
            static_cast<float>(voxelmap::kOcclusionMaskLevels));
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "lampFaceSide",
            static_cast<float>(light::kShadowFaceResolution));
        viewportRenderer.setShaderNumber(
            *voxelShader, "lampBias", light::kLampShadowBias);
        viewportRenderer.setShaderNumber(
            *voxelShader, "walkerLightRange", light::kWalkerLightRange);

        for (const auto &[face, uniformName] :
             {std::pair{gfx::CubeFace::East, "lampViewEast"},
              std::pair{gfx::CubeFace::West, "lampViewWest"},
              std::pair{gfx::CubeFace::Up, "lampViewUp"},
              std::pair{gfx::CubeFace::Down, "lampViewDown"},
              std::pair{gfx::CubeFace::South, "lampViewSouth"},
              std::pair{gfx::CubeFace::North, "lampViewNorth"}})
        {
            viewportRenderer.setShaderMatrix(
                *voxelShader,
                uniformName,
                light::shadowCamera(gfx::Vec3{}, face)
                    .viewProjection());
        }
    }

    gfx::IShader &WorldShader::program() const noexcept
    {
        return *voxelShader;
    }

    void WorldShader::setLook(
        gfx::IRenderer &viewportRenderer,
        const WorldShaderInputs &shaderInputs,
        const std::vector<light::ActiveLight> &lights,
        const std::span<const light::ActiveLight> bakedLights) const
    {
        viewportRenderer.setShaderVector(
            *voxelShader,
            "hidingCorner",
            gfx::Vec3{
                static_cast<float>(shaderInputs.hidingCornerPosition.x),
                0.0F,
                static_cast<float>(shaderInputs.hidingCornerPosition.z)});
        const auto sightSlot = shaderInputs.sightSlot;
        const auto upperSightSlot = shaderInputs.upperSightSlot;

        viewportRenderer.setShaderVector(
            *voxelShader,
            "sightPoint",
            sightOrigin(
                sightSlot, shaderInputs.sightPoint, lights, bakedLights));
        viewportRenderer.setShaderVector(
            *voxelShader,
            "upperSightPoint",
            shaderInputs.upperSightOn
                ? sightOrigin(
                      upperSightSlot,
                      shaderInputs.upperSightPoint,
                      lights,
                      bakedLights)
                : shaderInputs.upperSightPoint);
        viewportRenderer.setShaderNumber(
            *voxelShader, "sightOn", shaderInputs.sightOn ? 1.0F : 0.0F);
        viewportRenderer.setShaderNumber(
            *voxelShader, "sightSlot", static_cast<float>(sightSlot));
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "upperSightSlot",
            static_cast<float>(upperSightSlot));
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "upperSightOn",
            shaderInputs.upperSightOn ? 1.0F : 0.0F);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "lightAmbient",
            shaderInputs.lighting ? shaderInputs.ambient : 1.0F);
        viewportRenderer.setShaderNumber(
            *voxelShader, "nightEdge", shaderInputs.playing ? 1.0F : 0.0F);
        viewportRenderer.setShaderVector(
            *voxelShader, "fadeFrom", shaderInputs.walkerPosition);
        viewportRenderer.setShaderNumber(
            *voxelShader, "fadeReach", light::kNightRange);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "lampStrength",
            shaderInputs.lighting ? light::kLampStrength : 0.0F);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "lampsLit",
            shaderInputs.lighting ? static_cast<float>(lights.size()) : 0.0F);
        viewportRenderer.setShaderNumber(
            *voxelShader, "walkerAt", shaderInputs.fadeAbove);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "walkerLight",
            shaderInputs.lighting && shaderInputs.playing
                ? light::kWalkerLight
                : 0.0F);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "carrying",
            shaderInputs.carrying.has_value()
                ? static_cast<float>(*shaderInputs.carrying)
                : -1.0F);

        for (std::size_t index = 0; index < lights.size(); ++index)
        {
            viewportRenderer.setShaderVector(
                *voxelShader,
                kLampAtNames.at(index),
                lights.at(index).castsShadows && index < bakedLights.size()
                    ? bakedLights[index].position
                    : lights.at(index).position);
            viewportRenderer.setShaderColor(
                *voxelShader,
                kLampTintNames.at(index),
                lights.at(index).tintColor);
            viewportRenderer.setShaderNumber(
                *voxelShader,
                kLampReachNames.at(index),
                lights.at(index).reach);
            viewportRenderer.setShaderNumber(
                *voxelShader,
                kLampBrightnessNames.at(index),
                lights.at(index).brightness);
            viewportRenderer.setShaderNumber(
                *voxelShader,
                kLampShadowNames.at(index),
                lights.at(index).castsShadows ? 1.0F : 0.0F);
        }
    }

}
