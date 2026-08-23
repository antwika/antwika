#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include <antwika/component/CarriedLight.hpp>
#include <antwika/component/FillLight.hpp>
#include <antwika/component/Lamplight.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/ecs/World.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

#include <antwika/light/ActiveLight.hpp>

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::component::kCarriedLightHeight;
using antwika::light::kMaxLamps;
using antwika::light::getCarriedLightSlot;
using antwika::light::Lamp;
using antwika::light::ActiveLight;
using antwika::light::getDirtyShadowSlots;
using antwika::light::getActiveLights;
using antwika::component::CarriedLight;
using antwika::component::FillLight;
using antwika::component::kFillLightHeight;
using antwika::component::Position;
using antwika::voxel::VoxelCell;
using antwika::voxel::VoxelPosition;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr float kTolerance = 0.0001F;

    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

}

TEST(ActiveLightTest, ActiveLights_TakesEveryLampSetDown)
{
    NiceMock<MockLogger> logger;
    const World world(logger);
    const std::vector<Lamp> lamps{
        Lamp{.position = VoxelPosition{.x = 1, .y = 2, .z = 3},
            .tintColor = kRedColor}};

    const auto lights = getActiveLights(world, lamps);

    ASSERT_EQ(lights.size(), 1U);
    EXPECT_NEAR(lights.front().position.y, 2.5F, kTolerance);
    EXPECT_EQ(lights.front().tintColor, kRedColor);
}

TEST(
    ActiveLightTest,
    ActiveLights_HangsACarriedLightOverWhatCarriesIt)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(
            entity, Position{.x = 4.0F, .y = 0.5F, .z = -2.0F});
        world.add<CarriedLight>(entity, CarriedLight{.tintColor = kRedColor});
    }

    const auto lights = getActiveLights(world, {});

    ASSERT_EQ(lights.size(), 1U);
    EXPECT_NEAR(lights.front().position.x, 4.0F, kTolerance);
    EXPECT_NEAR(
        lights.front().position.y,
        0.5F + kCarriedLightHeight,
        kTolerance);
    EXPECT_NEAR(lights.front().position.z, -2.0F, kTolerance);
}

TEST(
    ActiveLightTest,
    ActiveLights_CountsACarriedLightBeforeALampSetDown)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{});
        world.add<CarriedLight>(entity, CarriedLight{.tintColor = kRedColor});
    }

    std::vector<Lamp> lamps;

    for (std::size_t index = 0; index < kMaxLamps; ++index)
    {
        lamps.push_back(
            Lamp{
                .position = VoxelPosition{.x = static_cast<std::int32_t>(
                    index)}});
    }

    const auto lights = getActiveLights(world, lamps);

    ASSERT_EQ(lights.size(), kMaxLamps);
    EXPECT_EQ(lights.front().tintColor, kRedColor);
}

TEST(
    ActiveLightTest,
    ActiveLights_GivesBackNoMoreThanTheWorldDrawsBy)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    {
        const OpenPhase phase(world);

        for (std::size_t index = 0; index <= kMaxLamps; ++index)
        {
            const auto entity = world.create();

            world.add<Position>(entity, Position{});
            world.add<CarriedLight>(entity, CarriedLight{});
        }
    }

    EXPECT_EQ(getActiveLights(world, {}).size(), kMaxLamps);
}

namespace
{

    [[nodiscard]] ActiveLight lightAt(const float east)
    {
        return ActiveLight{
            .position = antwika::gfx::Vec3{east, 1.0F, 2.0F},
            .tintColor = kRedColor};
    }

}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_AsksForNothingWhenNothingStirred)
{
    const std::vector<ActiveLight> lights{
        lightAt(1.0F), lightAt(2.0F)};

    EXPECT_TRUE(getDirtyShadowSlots(lights, lights).empty());
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_AsksForTheSlotOfALightThatMoved)
{
    const std::vector<ActiveLight> bakedLights{
        lightAt(1.0F), lightAt(2.0F)};
    const std::vector<ActiveLight> currentLights{
        lightAt(1.0F), lightAt(5.0F)};

    EXPECT_EQ(
        getDirtyShadowSlots(bakedLights, currentLights),
        (std::vector<std::size_t>{1}));
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_AsksForTheSlotOfALightRecolored)
{
    const std::vector<ActiveLight> bakedLights{lightAt(1.0F)};
    auto currentLights = bakedLights;

    currentLights.at(0).tintColor =
        antwika::gfx::Color{
            .red = 0, .green = 255, .blue = 0, .alpha = 255};

    EXPECT_EQ(
        getDirtyShadowSlots(bakedLights, currentLights),
        (std::vector<std::size_t>{0}));
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_AsksForEverySlotWhenTheSheetIsBare)
{
    const std::vector<ActiveLight> currentLights{
        lightAt(1.0F), lightAt(2.0F), lightAt(3.0F)};

    EXPECT_EQ(
        getDirtyShadowSlots({}, currentLights),
        (std::vector<std::size_t>{0, 1, 2}));
}

TEST(ActiveLightTest, DirtyShadowSlots_AsksForALightNewlyLit)
{
    const std::vector<ActiveLight> bakedLights{lightAt(1.0F)};
    const std::vector<ActiveLight> currentLights{
        lightAt(1.0F), lightAt(2.0F)};

    EXPECT_EQ(
        getDirtyShadowSlots(bakedLights, currentLights),
        (std::vector<std::size_t>{1}));
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_LooksNoFurtherThanTheLightsLeftLit)
{
    const std::vector<ActiveLight> bakedLights{
        lightAt(1.0F), lightAt(2.0F)};
    const std::vector<ActiveLight> currentLights{lightAt(1.0F)};

    EXPECT_TRUE(getDirtyShadowSlots(bakedLights, currentLights).empty());
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_LeavesTheLampsAWalkerPassesAlone)
{
    const std::vector<ActiveLight> bakedLights{
        lightAt(1.0F), lightAt(2.0F), lightAt(3.0F)};
    auto currentLights = bakedLights;

    currentLights.at(0).position.x += antwika::light::kShadowRedrawDistance;

    EXPECT_EQ(
        getDirtyShadowSlots(bakedLights, currentLights),
        (std::vector<std::size_t>{0}));
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_LeavesALampDriftedUnderAStepAlone)
{
    const std::vector<ActiveLight> bakedLights{
        lightAt(1.0F), lightAt(2.0F), lightAt(3.0F)};
    auto currentLights = bakedLights;

    currentLights.at(0).position.x +=
        antwika::light::kShadowRedrawDistance * 0.5F;

    EXPECT_EQ(
        getDirtyShadowSlots(bakedLights, currentLights),
        (std::vector<std::size_t>{}));
}

TEST(
    ActiveLightTest,
    CarriedLightSlot_FindsNothingForAThingCarryingNoLight)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto bareEntity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(bareEntity, Position{});
    }

    EXPECT_FALSE(getCarriedLightSlot(world, bareEntity).has_value());
}

TEST(
    ActiveLightTest,
    CarriedLightSlot_GivesTheSlotActiveLightsLaidTheLightIn)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto first = world.create();
    const auto second = world.create();

    {
        const OpenPhase phase(world);

        for (const auto entity : {first, second})
        {
            world.add<Position>(entity, Position{.x = 1.0F});
            world.add<CarriedLight>(
                entity, CarriedLight{.tintColor = kRedColor});
        }
    }

    const auto slot = getCarriedLightSlot(world, second);
    const auto lights = getActiveLights(world, {});

    ASSERT_TRUE(slot.has_value());
    ASSERT_LT(*slot, lights.size());
    EXPECT_EQ(
        getCarriedLightSlot(world, first),
        std::optional<std::size_t>{0});
    EXPECT_EQ(slot, std::optional<std::size_t>{1});
}

TEST(
    ActiveLightTest,
    CarriedLightSlot_FindsNothingPastTheSheetsLastSlot)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    std::vector<antwika::ecs::Entity> litEntities;

    {
        const OpenPhase phase(world);

        for (std::size_t index = 0; index <= kMaxLamps; ++index)
        {
            const auto entity = world.create();

            world.add<Position>(entity, Position{});
            world.add<CarriedLight>(entity, CarriedLight{});
            litEntities.push_back(entity);
        }
    }

    EXPECT_FALSE(getCarriedLightSlot(world, litEntities.back()).has_value());
}

TEST(
    ActiveLightTest,
    CarriedLightHeight_HangsALightClearOfEveryPlanesSeam)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();
    const auto top = 0.5F * antwika::voxel::kVoxelSide;

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{.x = 0.0F, .y = top});
        world.add<CarriedLight>(entity, CarriedLight{});
    }

    const auto lights = getActiveLights(world, {});

    ASSERT_EQ(lights.size(), 1U);

    const auto levelsUp =
        lights.front().position.y / antwika::voxel::kVoxelSide;
    const auto onSeam = levelsUp + 0.5F;
    const auto toSeam = std::abs(onSeam - std::round(onSeam));

    EXPECT_GT(lights.front().position.y, top + antwika::voxel::kVoxelSide);
    EXPECT_GT(toSeam, 0.25F);
}

TEST(
    ActiveLightTest,
    ActiveLights_CarriesALightFurtherThanOneSetDown)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{});
        world.add<CarriedLight>(entity, CarriedLight{});
    }

    const std::vector<Lamp> lamps{
        Lamp{.position = VoxelPosition{.x = 6, .y = 0, .z = 0}}};
    const auto lights = getActiveLights(world, lamps);

    ASSERT_EQ(lights.size(), 2U);
    EXPECT_NEAR(
        lights.front().reach,
        antwika::component::kCarriedLightRange,
        kTolerance);
    EXPECT_NEAR(
        lights.back().reach,
        antwika::component::kLampRange,
        kTolerance);
    EXPECT_GT(
        antwika::component::kCarriedLightRange,
        antwika::component::kLampRange);
}

TEST(
    ActiveLightTest,
    CarriedLightRange_StaysWithinWhatALampsPassLooks)
{
    EXPECT_LE(
        antwika::component::kCarriedLightRange,
        antwika::light::kSightRange);
}

TEST(ActiveLightTest, ActiveLights_CountsFolkAfterTheCarriedLight)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{});
        world.add<CarriedLight>(entity, CarriedLight{.tintColor = kRedColor});
    }

    const std::vector<ActiveLight> folkLights{
        ActiveLight{.position = {1.0F, 2.0F, 3.0F}}};
    const auto lights = getActiveLights(world, folkLights, {});

    ASSERT_EQ(lights.size(), 2U);
    EXPECT_EQ(lights.front().tintColor, kRedColor);
    EXPECT_NEAR(lights.back().position.x, 1.0F, kTolerance);
}

TEST(ActiveLightTest, ActiveLights_CountsFolkBeforeALampSetDown)
{
    const std::vector<ActiveLight> folkLights{
        ActiveLight{.position = {1.0F, 2.0F, 3.0F}, .tintColor = kRedColor}};
    std::vector<Lamp> lamps;

    for (std::size_t index = 0; index < kMaxLamps; ++index)
    {
        lamps.push_back(
            Lamp{
                .position = VoxelPosition{.x = static_cast<std::int32_t>(
                    index)}});
    }

    const auto lights = getActiveLights(folkLights, lamps);

    ASSERT_EQ(lights.size(), kMaxLamps);
    EXPECT_EQ(lights.front().tintColor, kRedColor);
}

TEST(ActiveLightTest, ActiveLights_MatchesItsOldWaysWithNoFolk)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{});
        world.add<CarriedLight>(entity, CarriedLight{});
    }

    const std::vector<Lamp> lamps{
        Lamp{.position = VoxelPosition{.x = 4}}};

    EXPECT_EQ(
        getActiveLights(world, lamps), getActiveLights(world, {}, lamps));
    EXPECT_EQ(
        getActiveLights(lamps),
        getActiveLights(std::vector<ActiveLight>{}, lamps));
}

TEST(
    ActiveLightTest,
    ActiveLights_HangsAFillLightOverWhatItIsHungOn)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(
            entity, Position{.x = 4.0F, .y = 0.5F, .z = -2.0F});
        world.add<FillLight>(entity, FillLight{});
    }

    const auto lights = getActiveLights(world, {});

    ASSERT_EQ(lights.size(), 1U);
    EXPECT_NEAR(lights.front().position.x, 4.0F, kTolerance);
    EXPECT_NEAR(
        lights.front().position.y,
        0.5F + kFillLightHeight,
        kTolerance);
    EXPECT_NEAR(lights.front().position.z, -2.0F, kTolerance);
    EXPECT_FALSE(lights.front().castsShadows);
}

TEST(ActiveLightTest, ActiveLights_HangsAFillLightOverACarriedOne)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{});
        world.add<CarriedLight>(entity, CarriedLight{});
        world.add<FillLight>(entity, FillLight{});
    }

    const auto lights = getActiveLights(world, {});

    ASSERT_EQ(lights.size(), 2U);
    EXPECT_GT(lights.back().position.y, lights.front().position.y);
}

TEST(
    ActiveLightTest,
    FillLightHeight_HangsWellOverTheFloorAStairLeadsTo)
{
    constexpr auto kStepUp = antwika::voxel::kVoxelSide;

    const auto carriedClears =
        antwika::component::kCarriedLightHeight - kStepUp;
    const auto fillClears = kFillLightHeight - kStepUp;

    EXPECT_GT(fillClears, 0.0F);
    EXPECT_GE(fillClears, 4.0F * carriedClears);
}

TEST(
    ActiveLightTest,
    FillLightRange_ReachesPastALampWithoutLeakingAsFarAsACarriedLight)
{
    EXPECT_GT(
        antwika::component::kFillLightRange,
        antwika::component::kLampRange);
    EXPECT_LT(
        antwika::component::kFillLightRange,
        antwika::component::kCarriedLightRange);
}

TEST(ActiveLightTest, ActiveLights_BurnsAFillLightDimmerThanALamp)
{
    EXPECT_LT(
        antwika::component::kFillLightTintColor.red,
        antwika::component::kLampTintColor.red);
    EXPECT_LT(
        antwika::component::kFillLightTintColor.green,
        antwika::component::kLampTintColor.green);
    EXPECT_LT(
        antwika::component::kFillLightTintColor.blue,
        antwika::component::kLampTintColor.blue);
}

TEST(
    ActiveLightTest,
    ActiveLights_CountsAFillLightAfterEveryCarriedOne)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto first = world.create();
    const auto second = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(first, Position{});
        world.add<FillLight>(first, FillLight{});
        world.add<Position>(second, Position{});
        world.add<CarriedLight>(second, CarriedLight{.tintColor = kRedColor});
    }

    const auto lights = getActiveLights(world, {});

    ASSERT_EQ(lights.size(), 2U);
    EXPECT_EQ(lights.front().tintColor, kRedColor);
    EXPECT_EQ(
        getCarriedLightSlot(world, second),
        std::optional<std::size_t>{0});
}

TEST(ActiveLightTest, ActiveLights_CountsAFillLightBeforeTheFolk)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto entity = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(entity, Position{});
        world.add<FillLight>(entity, FillLight{});
    }

    const std::vector<ActiveLight> folkLights{
        ActiveLight{.position = {1.0F, 2.0F, 3.0F}, .tintColor = kRedColor}};
    const auto lights = getActiveLights(world, folkLights, {});

    ASSERT_EQ(lights.size(), 2U);
    EXPECT_FALSE(lights.front().castsShadows);
    EXPECT_EQ(lights.back().tintColor, kRedColor);
}

TEST(
    ActiveLightTest,
    ActiveLights_GivesBackNoMoreThanTheWorldDrawsByWithFillLights)
{
    NiceMock<MockLogger> logger;
    World world(logger);

    {
        const OpenPhase phase(world);

        for (std::size_t index = 0; index <= kMaxLamps; ++index)
        {
            const auto entity = world.create();

            world.add<Position>(entity, Position{});
            world.add<CarriedLight>(entity, CarriedLight{});
            world.add<FillLight>(entity, FillLight{});
        }
    }

    EXPECT_EQ(getActiveLights(world, {}).size(), kMaxLamps);
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_NeverAsksForALightThatCastsNoShadow)
{
    auto currentLights = std::vector<ActiveLight>{
        lightAt(1.0F), lightAt(2.0F), lightAt(3.0F)};

    currentLights.at(1).castsShadows = false;

    EXPECT_EQ(
        getDirtyShadowSlots({}, currentLights),
        (std::vector<std::size_t>{0, 2}));
}

TEST(
    ActiveLightTest,
    DirtyShadowSlots_LeavesAShadowlessLightThatMovedAlone)
{
    auto bakedLights = std::vector<ActiveLight>{lightAt(1.0F)};
    auto currentLights = bakedLights;

    bakedLights.at(0).castsShadows = false;
    currentLights.at(0).castsShadows = false;
    currentLights.at(0).position.x = 9.0F;

    EXPECT_TRUE(getDirtyShadowSlots(bakedLights, currentLights).empty());
}

TEST(
    ActiveLightTest,
    ActiveLights_CountsEveryCarriedLightBeforeTheFillLights)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto carrying = world.create();
    const auto hanging = world.create();

    {
        const OpenPhase phase(world);

        world.add<Position>(carrying, Position{.x = 1.0F});
        world.add<CarriedLight>(
            carrying, CarriedLight{.tintColor = kRedColor});
        world.add<Position>(hanging, Position{.x = 2.0F});
        world.add<FillLight>(hanging, FillLight{});
        world.add<CarriedLight>(hanging, CarriedLight{});
    }

    const auto lights = getActiveLights(world, {});

    ASSERT_EQ(lights.size(), 3U);
    EXPECT_TRUE(lights.at(0).castsShadows);
    EXPECT_TRUE(lights.at(1).castsShadows);
    EXPECT_FALSE(lights.at(2).castsShadows);
}

TEST(
    ActiveLightTest,
    CarriedLightSlot_KeepsACarriersSlotWhenAnotherIsTakenAway)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    const auto first = world.create();
    const auto second = world.create();
    const auto third = world.create();

    {
        const OpenPhase phase(world);

        for (const auto entity : {first, second, third})
        {
            world.add<Position>(entity, Position{});
            world.add<CarriedLight>(entity, CarriedLight{});
        }
    }

    {
        const OpenPhase phase(world);

        world.destroy(second);
    }

    EXPECT_EQ(
        getCarriedLightSlot(world, first),
        std::optional<std::size_t>{0});
    EXPECT_EQ(
        getCarriedLightSlot(world, third),
        std::optional<std::size_t>{1});
}
