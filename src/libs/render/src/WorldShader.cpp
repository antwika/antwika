#include "antwika/render/WorldShader.hpp"

#include <glm/geometric.hpp>

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/gfx/CubeFace.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/voxelmap/Voxel.hpp>

namespace antwika::render
{

    namespace
    {
        [[nodiscard]] gfx::Vec3 getSightOrigin(
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

        template <std::size_t kLength>
        struct SlotUniformNames final
        {
            static_assert(light::kMaxLamps <= 10);

            std::array<std::array<char, kLength + 2>, light::kMaxLamps>
                characters{};

            std::array<std::string_view, light::kMaxLamps> names{};

            explicit constexpr SlotUniformNames(
                const char (&prefix)[kLength])
            {
                for (std::size_t slot = 0;
                     slot < light::kMaxLamps;
                     ++slot)
                {
                    auto &name = characters.at(slot);

                    for (std::size_t index = 0;
                         index + 1 < kLength;
                         ++index)
                    {
                        name.at(index) = prefix[index];
                    }

                    name.at(kLength - 1) = '[';
                    name.at(kLength) =
                        static_cast<char>('0' + slot);
                    name.at(kLength + 1) = ']';
                    names.at(slot) =
                        std::string_view{name.data(), name.size()};
                }
            }
        };

        constexpr SlotUniformNames kLampAtNames{"lampAt"};

        constexpr SlotUniformNames kLampReachNames{"lampReach"};

        constexpr SlotUniformNames kLampBrightnessNames{"lampBrightness"};

        constexpr SlotUniformNames kLampTintNames{"lampTint"};

        constexpr SlotUniformNames kLampShadowNames{"lampShadows"};

        constexpr gfx::Vec3 kAheadWayVector{0.0F, 0.0F, -1.0F};

        [[nodiscard]] gfx::Vec3 getViewWayVector(
            const gfx::Vec3 viewPosition, const gfx::Vec3 viewTargetPoint)
        {
            const auto spanVector = viewTargetPoint - viewPosition;

            return glm::length(spanVector) > 0.0001F
                       ? glm::normalize(spanVector)
                       : kAheadWayVector;
        }
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
            *voxelShader,
            "lampSlots",
            static_cast<float>(light::kMaxLamps));
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "lampFaces",
            static_cast<float>(gfx::kCubeFaces));
        viewportRenderer.setShaderNumber(
            *voxelShader, "lampBias", light::kLampShadowBias);
        viewportRenderer.setShaderNumber(
            *voxelShader, "walkerLightRange", light::kWalkerLightRange);
        viewportRenderer.setShaderNumber(*voxelShader, "fogNear", kFogNear);
        viewportRenderer.setShaderNumber(*voxelShader, "fogFar", kFogFar);

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
                light::getShadowCamera(gfx::Vec3{}, face)
                    .getViewProjection());
        }
    }

    gfx::IShader &WorldShader::getProgram() const noexcept
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
            getSightOrigin(
                sightSlot, shaderInputs.sightPoint, lights, bakedLights));
        viewportRenderer.setShaderVector(
            *voxelShader,
            "upperSightPoint",
            shaderInputs.upperSightOn
                ? getSightOrigin(
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
        viewportRenderer.setShaderVector(
            *voxelShader, "fogFrom", shaderInputs.viewTargetPoint);
        viewportRenderer.setShaderVector(
            *voxelShader,
            "fogWay",
            getViewWayVector(
                shaderInputs.viewPosition, shaderInputs.viewTargetPoint));
        viewportRenderer.setShaderColor(
            *voxelShader, "fogTint", shaderInputs.backdropColor);
        viewportRenderer.setShaderNumber(
            *voxelShader,
            "fogStrength",
            shaderInputs.playing ? kFogStrength : 0.0F);
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
                kLampAtNames.names.at(index),
                lights.at(index).castsShadows && index < bakedLights.size()
                    ? bakedLights[index].position
                    : lights.at(index).position);
            viewportRenderer.setShaderColor(
                *voxelShader,
                kLampTintNames.names.at(index),
                lights.at(index).tintColor);
            viewportRenderer.setShaderNumber(
                *voxelShader,
                kLampReachNames.names.at(index),
                lights.at(index).reach);
            viewportRenderer.setShaderNumber(
                *voxelShader,
                kLampBrightnessNames.names.at(index),
                lights.at(index).brightness);
            viewportRenderer.setShaderNumber(
                *voxelShader,
                kLampShadowNames.names.at(index),
                lights.at(index).castsShadows ? 1.0F : 0.0F);
        }
    }

}
