#include "antwika/render/Sprites.hpp"

#include <cstddef>
#include <numbers>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/MeshMaterial.hpp>

namespace antwika::render
{

    namespace
    {
        [[nodiscard]] gfx::MeshData getShadowSpot()
        {
            constexpr std::size_t kSpokes = 20;
            constexpr float kAcross =
                8.0F * collision::kWalkerPixel * 0.9F;
            constexpr float kAlong =
                6.0F * collision::kWalkerPixel * 0.9F;

            gfx::MeshData mesh;

            mesh.vertices.push_back(
                gfx::Vertex3D{
                    .position = {0.0F, 0.0F, 0.0F},
                    .normal = {0.0F, 1.0F, 0.0F},
                    .color = {.red = 0,
                              .green = 0,
                              .blue = 0,
                              .alpha = 140}});

            for (std::size_t index = 0; index <= kSpokes; ++index)
            {
                const auto turn =
                    2.0F * std::numbers::pi_v<float>
                    * static_cast<float>(index)
                    / static_cast<float>(kSpokes);

                mesh.vertices.push_back(
                    gfx::Vertex3D{
                        .position =
                            {std::cos(turn) * kAcross,
                             0.0F,
                             std::sin(turn) * kAlong},
                        .normal = {0.0F, 1.0F, 0.0F},
                        .color = {.red = 0,
                                  .green = 0,
                                  .blue = 0,
                                  .alpha = 0}});
            }

            for (std::uint32_t index = 1; index <= kSpokes; ++index)
            {
                mesh.indices.push_back(0);
                mesh.indices.push_back(index);
                mesh.indices.push_back(index + 1);
                mesh.indices.push_back(0);
                mesh.indices.push_back(index + 1);
                mesh.indices.push_back(index);
            }

            return mesh;
        } // GCOVR_EXCL_LINE
    }

    void Sprites::open(gfx::IRenderer &viewportRenderer)
    {
        figureMesh = viewportRenderer.createMesh(character::getCharacterMesh());
        shadowBlobMesh = viewportRenderer.createMesh(getShadowSpot());
    }

    void Sprites::drawCharacter(
        gfx::IRenderer &viewportRenderer,
        gfx::IShader &shader,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        gfx::ITexture *const sheetTexture,
        const component::Position position,
        const component::AnimationState posedState,
        const time::Tick tick,
        const gfx::ITexture *const lampShadowTexture) const
    {
        const gfx::Vec3 localPosition{position.x, position.y, position.z};
        const auto frame = character::getCurrentFrame(posedState, tick);

        viewportRenderer.setShaderNumber(shader, "spriteLit", 1.0F);
        viewportRenderer.setShaderVector(shader, "spriteAt", localPosition);
        viewportRenderer.setShaderVector(
            shader,
            "spriteFrom",
            character::getFrameUvOffset(
                frame / character::kCharacterFrames,
                frame % character::kCharacterFrames));
        viewportRenderer.setShaderVector(
            shader, "spriteSpan", character::getFrameUvSize());
        viewportRenderer.drawMesh(
            *figureMesh,
            character::getSpriteBillboardMatrix(
                gfx::Vec3(modelMatrix * gfx::Vec4(localPosition, 1.0F)),
                camera.getView()),
            camera,
            gfx::MeshMaterial{
                .texture = sheetTexture,
                .pointLightShadowAtlasTexture = lampShadowTexture,
                .shader = &shader});
        viewportRenderer.setShaderNumber(shader, "spriteLit", 0.0F);
    }

    void Sprites::drawShadow(
        gfx::IRenderer &viewportRenderer,
        const gfx::Camera3D &camera,
        const gfx::Mat4 &modelMatrix,
        const gfx::Vec3 position) const
    {
        auto liftMatrix = gfx::getIdentityMatrix();

        liftMatrix[3] = gfx::Vec4(position.x, position.y, position.z, 1.0F);
        viewportRenderer.drawMesh(
            *shadowBlobMesh, modelMatrix * liftMatrix, camera,
            gfx::MeshMaterial{});
    }

}
