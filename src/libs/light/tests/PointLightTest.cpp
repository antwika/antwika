#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <numbers>
#include <set>
#include <utility>
#include <vector>

#include <antwika/component/Lamplight.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/gfx/Math3D.hpp>

#include <antwika/light/PointLight.hpp>


namespace
{
    constexpr float kTolerance = 0.001F;
}

TEST(PointLightTest, WithLampAt_SetsOneDownWhereItIsAsked)
{
    using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;
    using antwika::light::withLampAt;

    constexpr VoxelPosition wherePosition{.x = 1, .y = 2, .z = 3};
    constexpr antwika::gfx::Color tintColor{
        .red = 10, .green = 20, .blue = 30, .alpha = 40};

    const auto lamps = withLampAt({}, wherePosition, tintColor);

    ASSERT_EQ(lamps.size(), 1U);
    EXPECT_EQ(lamps.front().position, wherePosition);
    EXPECT_EQ(lamps.front().tintColor, tintColor);
}

TEST(PointLightTest, WithLampAt_ColorsAfreshTheLampAlreadyThere)
{
    using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;
    using antwika::light::withLampAt;

    constexpr VoxelPosition wherePosition{.x = 4};
    constexpr antwika::gfx::Color mineColor{.red = 200};

    const auto lamps =
        withLampAt(withLampAt({}, wherePosition, {}), wherePosition, mineColor);

    ASSERT_EQ(lamps.size(), 1U);
    EXPECT_EQ(lamps.front().tintColor, mineColor);
}

TEST(PointLightTest, WithLampAt_SetsNoMoreDownThanTheWorldDrawsBy)
{
    using antwika::light::kMaxLamps;
    using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;
    using antwika::light::withLampAt;

    std::vector<antwika::light::Lamp> lamps;

    for (std::int32_t index = 0;
         index < static_cast<std::int32_t>(kMaxLamps) + 3;
         ++index)
    {
        lamps = withLampAt(lamps, VoxelPosition{.x = index}, {});
    }

    EXPECT_EQ(lamps.size(), kMaxLamps);
}

TEST(PointLightTest, WithoutLampAt_TakesAwayOnlyTheOneAsked)
{
    using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;
    using antwika::light::withLampAt;
    using antwika::light::withoutLampAt;

    constexpr VoxelPosition minePosition{.x = 1};
    constexpr VoxelPosition theirsPosition{.x = 2};

    const auto lamps =
        withoutLampAt(withLampAt(withLampAt({}, minePosition, {}),
                                 theirsPosition, {}),
                      minePosition);

    ASSERT_EQ(lamps.size(), 1U);
    EXPECT_EQ(lamps.front().position, theirsPosition);
}

TEST(PointLightTest, LampGizmoSpans_CrossesTheMiddleOfItsOwnPlace)
{
    using antwika::voxelmap::getCellMiddle;
    using antwika::light::Lamp;
    using antwika::light::getLampGizmoSpans;
    using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;

    constexpr Lamp lamp{.position = VoxelPosition{.x = 2, .y = -1, .z = 5}};

    const auto middle = getCellMiddle(lamp.position);
    const auto spans = getLampGizmoSpans(lamp);

    EXPECT_FALSE(spans.empty());

    for (const auto &span : spans)
    {
        EXPECT_NEAR((span.fromPosition.x + span.toPosition.x) / 2.0F,
                    middle.x, kTolerance);
        EXPECT_NEAR((span.fromPosition.y + span.toPosition.y) / 2.0F,
                    middle.y, kTolerance);
        EXPECT_NEAR((span.fromPosition.z + span.toPosition.z) / 2.0F,
                    middle.z, kTolerance);
    }
}

TEST(
    PointLightTest,
    ShadowAtlasSize_HoldsAPictureForEveryWayAndLamp)
{
    using antwika::light::kShadowFaceResolution;
    using antwika::light::kMaxLamps;
    using antwika::light::getShadowAtlasSize;

    const auto sheet = getShadowAtlasSize();

    EXPECT_EQ(
        sheet.width,
        kShadowFaceResolution
            * static_cast<std::uint32_t>(antwika::gfx::kCubeFaces));
    EXPECT_EQ(
        sheet.height,
        kShadowFaceResolution
            * static_cast<std::uint32_t>(kMaxLamps));
}

TEST(PointLightTest, ShadowFaceRect_GivesEveryPictureAPlaceOfItsOwn)
{
    using antwika::light::kMaxLamps;
    using antwika::light::getShadowFaceRect;
    using antwika::light::getShadowAtlasSize;

    const auto sheet = getShadowAtlasSize();
    std::set<std::pair<std::int32_t, std::int32_t>> corners;

    for (std::size_t lamp = 0; lamp < kMaxLamps; ++lamp)
    {
        for (const auto face : antwika::gfx::kEveryCubeFace)
        {
            const auto patch = getShadowFaceRect(lamp, face);

            EXPECT_TRUE(
                corners
                    .insert({patch.originPoint.x, patch.originPoint.y})
                    .second);
            EXPECT_LE(
                static_cast<std::uint32_t>(patch.originPoint.x)
                    + patch.size.width,
                sheet.width);
            EXPECT_LE(
                static_cast<std::uint32_t>(patch.originPoint.y)
                    + patch.size.height,
                sheet.height);
        }
    }

    EXPECT_EQ(
        corners.size(), kMaxLamps * antwika::gfx::kCubeFaces);
}

TEST(
    PointLightTest,
    ShadowFaceRect_CountsTheRowsFromTheTopOfTheSheetDown)
{
    using antwika::light::kShadowFaceResolution;
    using antwika::light::kMaxLamps;
    using antwika::light::getShadowFaceRect;

    const auto side =
        static_cast<std::int32_t>(kShadowFaceResolution);

    for (std::size_t lamp = 0; lamp < kMaxLamps; ++lamp)
    {
        for (const auto face : antwika::gfx::kEveryCubeFace)
        {
            const auto patch = getShadowFaceRect(lamp, face);

            EXPECT_EQ(
                patch.originPoint.x,
                static_cast<std::int32_t>(face) * side);
            EXPECT_EQ(
                patch.originPoint.y,
                static_cast<std::int32_t>(lamp) * side);
        }
    }
}

TEST(PointLightTest, ShadowCamera_StandsAtTheLampAndLooksItsWay)
{
    using antwika::light::getShadowCamera;

    constexpr antwika::gfx::Vec3 position{2.0F, 3.0F, 4.0F};

    for (const auto face : antwika::gfx::kEveryCubeFace)
    {
        const auto camera = getShadowCamera(position, face);
        const auto way = antwika::gfx::directionOf(face);

        EXPECT_NEAR(
            camera.getPosition().x, position.x, kTolerance);
        EXPECT_NEAR(
            camera.getPosition().y, position.y, kTolerance);
        EXPECT_NEAR(
            camera.getPosition().z, position.z, kTolerance);
        EXPECT_NEAR(
            camera.getTarget().x, position.x + way.x, kTolerance);
        EXPECT_NEAR(
            camera.getTarget().y, position.y + way.y, kTolerance);
        EXPECT_NEAR(
            camera.getTarget().z, position.z + way.z, kTolerance);
    }
}

TEST(PointLightTest, ShadowCamera_LooksNearerThanAWalkerCanPress)
{
    using antwika::collision::kFootprintDepth;
    using antwika::collision::kFootprintWidth;
    using antwika::light::kLampFarPlane;
    using antwika::light::kLampNearPlane;

    EXPECT_GT(kLampNearPlane, 0.0F);
    EXPECT_LT(kLampNearPlane, kFootprintDepth / 2.0F);
    EXPECT_LT(kLampNearPlane, kFootprintWidth / 2.0F);
    EXPECT_LT(kLampNearPlane, kLampFarPlane);
}

TEST(PointLightTest, LampNearPlane_ClearsTheWallAWalkerPressesInto)
{
    using antwika::collision::kFootprintDepth;
    using antwika::light::kLampNearPlane;
    using antwika::voxel::kVoxelSide;
    using antwika::collision::kWalkSpeed;
    using antwika::component::Position;
    using antwika::voxel::VoxelPosition;
using antwika::voxel::VoxelPosition;
    using antwika::collision::getMovedWithCollision;
    using antwika::component::Velocity;

    constexpr std::int32_t kWall = 2;
    constexpr auto kFace = static_cast<float>(kWall) * kVoxelSide;

    antwika::voxel::Voxels filledVoxels;

    for (std::int32_t x = -2; x <= 2; ++x)
    {
        for (std::int32_t z = -3; z <= kWall; ++z)
        {
            filledVoxels[VoxelPosition{.x = x, .y = 0, .z = z}] =
            antwika::voxel::VoxelMaterial{};
        }

        filledVoxels[VoxelPosition{.x = x, .y = 1, .z = kWall}] =
            antwika::voxel::VoxelMaterial{};
        filledVoxels[VoxelPosition{.x = x, .y = 2, .z = kWall}] =
            antwika::voxel::VoxelMaterial{};
    }

    for (const auto depth : {-1.0F, -1.03F, -1.05F, -1.07F})
    {
        Position stoodPosition{.x = 0.0F, .y = 1.0F, .z = depth};

        for (std::size_t step = 0; step < 200; ++step)
        {
            stoodPosition = getMovedWithCollision(
                filledVoxels, stoodPosition, Velocity{.velocityZ = 1.0F});
        }

        const auto gap = kFace - stoodPosition.z;

        EXPECT_GT(gap, kLampNearPlane);
        EXPECT_LT(gap, (kFootprintDepth / 2.0F) + kWalkSpeed);
    }
}

TEST(PointLightTest, SightRange_LooksFurtherThanALampCarries)
{
    using antwika::light::kLampFarPlane;
    using antwika::component::kLampRange;
    using antwika::light::kSightRange;

    EXPECT_GT(kSightRange, kLampRange);
    EXPECT_GE(kLampFarPlane, kSightRange);
}
