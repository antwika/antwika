#include <gtest/gtest.h>

#include <antwika/gfx/SizeF.hpp>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/system/AnimationSystem.hpp>

#include "antwika/editor/ui/CharacterView.hpp"
#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/gameplay/GameLoop.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::Phase;
using antwika::editor::characterAt;
using antwika::character::characterPixelAt;
using antwika::editor::getCharacterPlace;
using antwika::editor::getCharacterCanvasRect;
using antwika::editor::getCharacterDrawBounds;
using antwika::editor::getCharacterSheetBounds;
using antwika::character::kCharacterFrames;
using antwika::component::AnimationState;
using antwika::system::AnimationSystem;
using antwika::gameplay::GameLoop;
using antwika::component::Velocity;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr antwika::gfx::Size kCanvasSize{
        .width = 480, .height = 270};

}

TEST(CharacterTest, CharacterAt_FindsTheFrameAPointFallsOn)
{
    const auto where = getCharacterPlace(
        getCharacterSheetBounds(kCanvasSize), 3, 2);
    const antwika::gfx::PointF middlePoint{
        where.originPoint.x + (where.size.width / 2.0F),
        where.originPoint.y + (where.size.height / 2.0F)};

    EXPECT_EQ(
        characterAt(getCharacterSheetBounds(kCanvasSize), middlePoint),
        (3U * kCharacterFrames) + 2U);
}

TEST(CharacterTest, CharacterPlace_FollowsTheSheetRectItIsGiven)
{
    const auto restingRect = getCharacterSheetBounds(kCanvasSize);
    const antwika::gfx::RectF movedRect(
        antwika::gfx::PointF{
            restingRect.originPoint.x + 30.0F,
            restingRect.originPoint.y},
        restingRect.size);

    const auto restingPlace = getCharacterPlace(restingRect, 0, 0);
    const auto movedPlace = getCharacterPlace(movedRect, 0, 0);

    EXPECT_FLOAT_EQ(
        movedPlace.originPoint.x, restingPlace.originPoint.x + 30.0F);
    EXPECT_FLOAT_EQ(movedPlace.originPoint.y, restingPlace.originPoint.y);
}

TEST(CharacterTest, CharacterCanvasRect_KeepsTheCellShapeOfTheFrame)
{
    const antwika::gfx::RectF drawRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{60.0F, 400.0F});
    const auto where = getCharacterCanvasRect(drawRect);
    const auto cellWide = static_cast<float>(
        antwika::character::kCharacterCellSize.width);
    const auto cellTall = static_cast<float>(
        antwika::character::kCharacterCellSize.height);

    EXPECT_FLOAT_EQ(where.size.width, drawRect.size.width);
    EXPECT_NEAR(
        where.size.width / where.size.height, cellWide / cellTall, 0.001F);
}

TEST(CharacterTest, CharacterCanvasRect_TakesTheHeightOfAWideDrawingRect)
{
    const antwika::gfx::RectF drawRect(
        antwika::gfx::PointF{0.0F, 0.0F},
        antwika::gfx::SizeF{400.0F, 80.0F});
    const auto where = getCharacterCanvasRect(drawRect);
    const auto cellWide = static_cast<float>(
        antwika::character::kCharacterCellSize.width);
    const auto cellTall = static_cast<float>(
        antwika::character::kCharacterCellSize.height);

    EXPECT_LT(where.size.width, drawRect.size.width);
    EXPECT_NEAR(
        where.size.width / where.size.height, cellWide / cellTall, 0.001F);
    EXPECT_LE(
        where.originPoint.y + where.size.height,
        drawRect.originPoint.y + drawRect.size.height);
}

TEST(CharacterTest, CharacterCanvasRect_HangsFromTheTopOfTheDrawingRect)
{
    const antwika::gfx::RectF drawRect(
        antwika::gfx::PointF{100.0F, 20.0F},
        antwika::gfx::SizeF{200.0F, 400.0F});
    const auto where = getCharacterCanvasRect(drawRect);

    EXPECT_GT(where.originPoint.y, drawRect.originPoint.y);
    EXPECT_LT(
        where.originPoint.y,
        drawRect.originPoint.y + (drawRect.size.height / 2.0F));
}

TEST(CharacterTest, CharacterCanvasRect_StandsAgainstTheRightOfTheRect)
{
    for (const float wide : {60.0F, 120.0F, 400.0F})
    {
        const antwika::gfx::RectF drawRect(
            antwika::gfx::PointF{100.0F, 20.0F},
            antwika::gfx::SizeF{wide, 200.0F});
        const auto where = getCharacterCanvasRect(drawRect);

        EXPECT_FLOAT_EQ(
            where.originPoint.x + where.size.width,
            drawRect.originPoint.x + drawRect.size.width);
    }
}

TEST(CharacterTest, CharacterCanvasRect_KeepsNoRoomInsideAnEmptyRect)
{
    const auto where = getCharacterCanvasRect(
        antwika::gfx::RectF(
            antwika::gfx::PointF{0.0F, 0.0F},
            antwika::gfx::SizeF{0.0F, 0.0F}));

    EXPECT_FLOAT_EQ(where.size.width, 0.0F);
    EXPECT_FLOAT_EQ(where.size.height, 0.0F);
}

TEST(CharacterTest, CharacterPlace_KeepsEveryFrameInsideTheSheetRect)
{
    const antwika::gfx::RectF sheetRect(
        antwika::gfx::PointF{40.0F, 30.0F},
        antwika::gfx::SizeF{90.0F, 200.0F});

    for (std::size_t way = 0; way < antwika::character::kCharacterWays;
         ++way)
    {
        for (std::size_t frame = 0; frame < kCharacterFrames; ++frame)
        {
            const auto place = getCharacterPlace(sheetRect, way, frame);

            EXPECT_GE(place.originPoint.x, sheetRect.originPoint.x - 0.01F);
            EXPECT_GE(place.originPoint.y, sheetRect.originPoint.y - 0.01F);
            EXPECT_LE(
                place.originPoint.x + place.size.width,
                sheetRect.originPoint.x + sheetRect.size.width + 0.01F);
            EXPECT_LE(
                place.originPoint.y + place.size.height,
                sheetRect.originPoint.y + sheetRect.size.height + 0.01F);
        }
    }
}

TEST(CharacterTest, CharacterAt_FindsNothingBesideTheSheetRect)
{
    const antwika::gfx::RectF sheetRect(
        antwika::gfx::PointF{40.0F, 30.0F},
        antwika::gfx::SizeF{90.0F, 200.0F});

    for (const antwika::gfx::PointF point :
         {antwika::gfx::PointF{300.0F, 100.0F},
          antwika::gfx::PointF{10.0F, 100.0F},
          antwika::gfx::PointF{80.0F, 250.0F},
          antwika::gfx::PointF{80.0F, 10.0F}})
    {
        EXPECT_FALSE(characterAt(sheetRect, point).has_value());
    }
}

TEST(CharacterTest, CharacterAt_FindsNothingInTheGapBetweenTwoFrames)
{
    const auto sheetRect = getCharacterSheetBounds(kCanvasSize);
    const auto firstRow = getCharacterPlace(sheetRect, 0, 0);
    const auto secondRow = getCharacterPlace(sheetRect, 1, 0);
    const auto gapMiddle =
        (firstRow.originPoint.y + firstRow.size.height
         + secondRow.originPoint.y)
        / 2.0F;

    ASSERT_GT(
        secondRow.originPoint.y,
        firstRow.originPoint.y + firstRow.size.height);
    EXPECT_FALSE(
        characterAt(
            sheetRect,
            antwika::gfx::PointF{
                firstRow.originPoint.x + (firstRow.size.width / 2.0F),
                gapMiddle})
            .has_value());
}

TEST(CharacterTest, CharacterCanvasRect_LaysThePixelGridOnTheCanvas)
{
    const auto where = getCharacterCanvasRect(
        getCharacterDrawBounds(
            kCanvasSize, antwika::editor::kRightPanelWidth));
    const auto pixel = characterPixelAt(
        where,
        antwika::gfx::PointF{
            where.originPoint.x + 1.0F, where.originPoint.y + 1.0F});

    EXPECT_GE(where.originPoint.x, 0.0F);
    EXPECT_GE(where.originPoint.y, 0.0F);
    EXPECT_LE(
        where.originPoint.x + where.size.width,
        static_cast<float>(kCanvasSize.width));
    EXPECT_LE(
        where.originPoint.y + where.size.height,
        static_cast<float>(kCanvasSize.height));
    ASSERT_TRUE(pixel.has_value());
    EXPECT_EQ(pixel->column, 0U);
    EXPECT_EQ(pixel->row, 0U);
    EXPECT_FALSE(
        characterPixelAt(
            where,
            antwika::gfx::PointF{
                where.originPoint.x - 1.0F, where.originPoint.y})
            .has_value());
}

TEST(CharacterTest, Update_TurnsAPoseTheWayItIsBeingSent)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    AnimationSystem posingSystem;

    gameLoop.addSystem(Phase::Walking, posingSystem);

    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Velocity>(entity, Velocity{.velocityZ = 1.0F});
        gameLoop.getWorld().add<AnimationState>(entity, AnimationState{});
    }

    gameLoop.run(7);

    const auto animationState = gameLoop.getWorld().get<AnimationState>(entity);

    EXPECT_EQ(animationState.direction, 2);
    EXPECT_TRUE(animationState.walking);
    EXPECT_EQ(animationState.startedAtTick, 7U);
}

TEST(CharacterTest, Update_KeepsTheWayAPoseFacedWhenItStops)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    AnimationSystem posingSystem;

    gameLoop.addSystem(Phase::Walking, posingSystem);

    const auto entity = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Velocity>(entity, Velocity{});
        gameLoop.getWorld().add<AnimationState>(
            entity,
            AnimationState{
                .direction = 5, .walking = true, .startedAtTick = 2});
    }

    gameLoop.run(9);

    const auto animationState = gameLoop.getWorld().get<AnimationState>(entity);

    EXPECT_EQ(animationState.direction, 5);
    EXPECT_FALSE(animationState.walking);
    EXPECT_EQ(animationState.startedAtTick, 9U);
}

TEST(CharacterTest, Update_PosesEveryCharacterInTheWorld)
{
    NiceMock<MockLogger> logger;
    World world(logger);
    GameLoop gameLoop(world);
    AnimationSystem posingSystem;

    gameLoop.addSystem(Phase::Walking, posingSystem);

    const auto walking = gameLoop.getWorld().create();
    const auto position = gameLoop.getWorld().create();

    {
        const OpenPhase phase(gameLoop.getWorld());

        gameLoop.getWorld().add<Velocity>(
            walking, Velocity{.velocityX = 1.0F});
        gameLoop.getWorld().add<AnimationState>(walking, AnimationState{});
        gameLoop.getWorld().add<Velocity>(position, Velocity{});
        gameLoop.getWorld().add<AnimationState>(
            position,
            AnimationState{
                .direction = 4, .walking = true, .startedAtTick = 1});
    }

    gameLoop.run(6);

    const auto animationState = gameLoop.getWorld().get<AnimationState>(walking);
    const auto restingState = gameLoop.getWorld().get<AnimationState>(position);

    EXPECT_EQ(animationState.direction, 0);
    EXPECT_TRUE(animationState.walking);
    EXPECT_EQ(animationState.startedAtTick, 6U);
    EXPECT_EQ(restingState.direction, 4);
    EXPECT_FALSE(restingState.walking);
    EXPECT_EQ(restingState.startedAtTick, 6U);
}
