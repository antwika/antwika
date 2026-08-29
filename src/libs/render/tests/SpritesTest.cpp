#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string_view>
#include <vector>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/Math3D.hpp>
#include <antwika/gfx/MeshData.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockShader.hpp>

#include "antwika/render/Sprites.hpp"

using antwika::render::Sprites;
using antwika::component::AnimationState;
using antwika::component::Position;
using antwika::gfx::Camera3D;
using antwika::gfx::IMesh;
using antwika::gfx::Mat4;
using antwika::gfx::Vec3;
using antwika::gfx::Vec4;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockShader;
using ::testing::NiceMock;

namespace
{
    void handsOutMeshes(NiceMock<MockRenderer> &innerRenderer)
    {
        ON_CALL(innerRenderer, createMesh(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::MeshData &data)
                {
                    return std::unique_ptr<IMesh>{
                        std::make_unique<NiceMock<MockMesh>>()};
                });
    }
}

TEST(SpritesTest, Open_TakesUpTheCharacterAndItsShadowBlob)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutMeshes(innerRenderer);
    Sprites sprites;

    EXPECT_CALL(innerRenderer, createMesh).Times(2);

    sprites.open(innerRenderer);
}

TEST(SpritesTest, DrawCharacter_DrawsItLitAndLeavesTheLightOff)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutMeshes(innerRenderer);
    NiceMock<MockShader> shader;
    Sprites sprites;
    std::vector<float> litValues;

    ON_CALL(
        innerRenderer,
        setShaderNumber(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            [&litValues](
                [[maybe_unused]] const antwika::gfx::IShader &target,
                const std::string_view name,
                const float value)
            {
                if (name == "spriteLit")
                {
                    litValues.push_back(value);
                }
            });

    sprites.open(innerRenderer);

    EXPECT_CALL(innerRenderer, drawMesh).Times(1);

    sprites.drawCharacter(
        innerRenderer,
        shader,
        Camera3D{},
        antwika::gfx::getIdentityMatrix(),
        nullptr,
        Position{},
        AnimationState{},
        0,
        nullptr);

    ASSERT_EQ(litValues.size(), 2U);
    EXPECT_EQ(litValues.at(0), 1.0F);
    EXPECT_EQ(litValues.at(1), 0.0F);
}

TEST(SpritesTest, DrawCharacter_PlacesTheSpriteAtThePosition)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutMeshes(innerRenderer);
    NiceMock<MockShader> shader;
    Sprites sprites;
    Vec3 spritePoint{};

    ON_CALL(
        innerRenderer,
        setShaderVector(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            [&spritePoint](
                [[maybe_unused]] const antwika::gfx::IShader &target,
                const std::string_view name,
                const Vec3 value)
            {
                if (name == "spriteAt")
                {
                    spritePoint = value;
                }
            });

    sprites.open(innerRenderer);
    sprites.drawCharacter(
        innerRenderer,
        shader,
        Camera3D{},
        antwika::gfx::getIdentityMatrix(),
        nullptr,
        Position{.x = 1.0F, .y = 2.0F, .z = 3.0F},
        AnimationState{},
        0,
        nullptr);

    EXPECT_EQ(spritePoint, (Vec3{1.0F, 2.0F, 3.0F}));
}

TEST(SpritesTest, DrawShadow_LiftsTheBlobToThePosition)
{
    NiceMock<MockRenderer> innerRenderer;
    handsOutMeshes(innerRenderer);
    Sprites sprites;
    Mat4 modelMatrix{};

    ON_CALL(
        innerRenderer,
        drawMesh(::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            [&modelMatrix](
                [[maybe_unused]] const antwika::gfx::IMesh &mesh,
                const Mat4 &model,
                [[maybe_unused]] const Camera3D &camera,
                [[maybe_unused]] const antwika::gfx::MeshMaterial
                    &material)
            {
                modelMatrix = model;
            });

    sprites.open(innerRenderer);
    sprites.drawShadow(
        innerRenderer,
        Camera3D{},
        antwika::gfx::getIdentityMatrix(),
        Vec3{4.0F, 5.0F, 6.0F});

    EXPECT_EQ(modelMatrix[3], (Vec4{4.0F, 5.0F, 6.0F, 1.0F}));
}
