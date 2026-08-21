#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/gfx/mocks/MockShader.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>

#include "antwika/gfx/BlendMode.hpp"
#include "antwika/gfx/MeshMaterial.hpp"

using antwika::gfx::MeshMaterial;
using antwika::gfx::mocks::MockShader;
using antwika::gfx::mocks::MockTexture;
using ::testing::NiceMock;

TEST(MeshMaterialTest, Default_AsksForTheRenderersOwnSurfacing)
{
    const MeshMaterial material;

    EXPECT_EQ(nullptr, material.texture);
    EXPECT_EQ(nullptr, material.materialMapTexture);
    EXPECT_EQ(nullptr, material.shader);
    EXPECT_EQ(antwika::gfx::BlendMode::Opaque, material.blend);
    EXPECT_EQ(255, material.tintColor.red);
    EXPECT_EQ(255, material.tintColor.green);
    EXPECT_EQ(255, material.tintColor.blue);
    EXPECT_EQ(255, material.tintColor.alpha);
}

TEST(MeshMaterialTest, OperatorEquals_ComparesEveryField)
{
    const NiceMock<MockTexture> texture;
    const NiceMock<MockShader> shader;

    const NiceMock<MockTexture> surfaceTexture;

    const MeshMaterial leftMaterial{
        .texture = &texture,
        .materialMapTexture = &surfaceTexture,
        .shader = &shader,
        .tintColor = {.red = 1, .green = 2, .blue = 3, .alpha = 4},
        .blend = antwika::gfx::BlendMode::Alpha};

    MeshMaterial rightMaterial = leftMaterial;

    EXPECT_EQ(leftMaterial, rightMaterial);

    rightMaterial.texture = nullptr;
    EXPECT_NE(leftMaterial, rightMaterial);

    rightMaterial = leftMaterial;
    rightMaterial.materialMapTexture = nullptr;
    EXPECT_NE(leftMaterial, rightMaterial);

    rightMaterial = leftMaterial;
    rightMaterial.shader = nullptr;
    EXPECT_NE(leftMaterial, rightMaterial);

    rightMaterial = leftMaterial;
    rightMaterial.tintColor.red = 9;
    EXPECT_NE(leftMaterial, rightMaterial);

    rightMaterial = leftMaterial;
    rightMaterial.blend = antwika::gfx::BlendMode::Opaque;
    EXPECT_NE(leftMaterial, rightMaterial);
}
