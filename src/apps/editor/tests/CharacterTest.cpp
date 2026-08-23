#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <set>
#include <tuple>

#include <antwika/component/AnimationState.hpp>
#include <antwika/component/Position.hpp>
#include <antwika/component/Velocity.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gfx/Camera3D.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/system/AnimationSystem.hpp>

#include "antwika/editor/ui/CharacterView.hpp"
#include "antwika/gameplay/GameLoop.hpp"

using antwika::ecs::OpenPhase;
using antwika::ecs::World;
using antwika::gameplay::Phase;
using antwika::character::getBlankCharacter;
using antwika::editor::characterAt;
using antwika::character::getCharacterPaletteColor;
using antwika::character::getCharacterCell;
using antwika::character::getCharacterPixel;
using antwika::character::characterPixelAt;
using antwika::editor::getCharacterPlace;
using antwika::character::getCharacterSheetSize;
using antwika::editor::getCharacterCanvasRect;
using antwika::character::getCurrentFrame;
using antwika::character::headTopOf;
using antwika::character::kCharacterCellSize;
using antwika::character::kCharacterFrames;
using antwika::character::kCharacterPaceTick;
using antwika::character::kCharacterWays;
using antwika::character::getCharacterPixelColor;
using antwika::character::paintCharacter;
using antwika::character::paintCharacterFill;
using antwika::component::AnimationState;
using antwika::system::AnimationSystem;
using antwika::gameplay::GameLoop;
using antwika::component::Velocity;
using antwika::character::getDirectionName;
using antwika::character::getFacingFromVelocity;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    constexpr antwika::gfx::Size kCanvasSize{
        .width = 480, .height = 270};

    [[nodiscard]] antwika::gfx::Mat4 getViewTiltedBy(const float pitch)
    {
        return antwika::gfx::Camera3D{
            antwika::gfx::Vec3{
                0.0F,
                -std::sin(pitch) * 8.0F,
                std::cos(pitch) * 8.0F},
            antwika::gfx::Vec3{0.0F, 0.0F, 0.0F},
            antwika::gfx::Vec3{0.0F, 1.0F, 0.0F},
            antwika::gfx::Orthographic{
                .halfWidth = 8.0F,
                .halfHeight = 8.0F,
                .nearPlane = -64.0F,
                .farPlane = 64.0F}}
            .getView();
    }

    [[nodiscard]] antwika::gfx::Mat4 getSquaredView()
    {
        return getViewTiltedBy(antwika::camera::getIsometricPitch());
    }

}

TEST(CharacterTest, CharacterSheetSize_HoldsAFrameForEveryWayAbout)
{
    const auto size = getCharacterSheetSize();

    EXPECT_EQ(size.width, kCharacterCellSize.width * kCharacterFrames);
    EXPECT_EQ(size.height, kCharacterCellSize.height * kCharacterWays);
}

TEST(CharacterTest, BlankCharacter_IsTheSizeTheSheetIs)
{
    const auto sheet = getBlankCharacter();

    EXPECT_TRUE(sheet.isValid());
    EXPECT_EQ(sheet.size, getCharacterSheetSize());
}

TEST(CharacterTest, CharacterCell_GivesEveryFrameAPlaceOfItsOwn)
{
    std::set<std::pair<std::int32_t, std::int32_t>> corners;

    for (std::size_t way = 0; way < kCharacterWays; ++way)
    {
        for (std::size_t frame = 0; frame < kCharacterFrames;
             ++frame)
        {
            const auto cell = getCharacterCell(way, frame);

            EXPECT_TRUE(
                corners.insert({cell.originPoint.x, cell.originPoint.y})
                    .second);
            EXPECT_LE(
                static_cast<std::uint32_t>(cell.originPoint.x)
                    + cell.size.width,
                getCharacterSheetSize().width);
            EXPECT_LE(
                static_cast<std::uint32_t>(cell.originPoint.y)
                    + cell.size.height,
                getCharacterSheetSize().height);
        }
    }
}

TEST(CharacterTest, DirectionName_NamesEveryWayAbout)
{
    std::set<std::string_view> names;

    for (std::size_t way = 0; way < kCharacterWays; ++way)
    {
        EXPECT_TRUE(names.insert(getDirectionName(way)).second);
    }
}

TEST(CharacterTest, FacingFromVelocity_TakesTheWayAHeadingFallsNearest)
{
    EXPECT_EQ(getFacingFromVelocity(Velocity{.velocityX = 1.0F}), 0U);
    EXPECT_EQ(
        getFacingFromVelocity(
            Velocity{.velocityX = 1.0F, .velocityZ = 1.0F}),
        1U);
    EXPECT_EQ(getFacingFromVelocity(Velocity{.velocityZ = 1.0F}), 2U);
    EXPECT_EQ(getFacingFromVelocity(Velocity{.velocityX = -1.0F}), 4U);
    EXPECT_EQ(getFacingFromVelocity(Velocity{.velocityZ = -1.0F}), 6U);
}

TEST(CharacterTest, FacingFromVelocity_GivesNothingForOneSentNowhere)
{
    EXPECT_FALSE(getFacingFromVelocity(Velocity{}).has_value());
}

TEST(CharacterTest, CurrentFrame_WalksTheFramesOfTheWayItFaces)
{
    const AnimationState posedState{
        .direction = 2, .walking = true, .startedAtTick = 0};

    EXPECT_EQ(getCurrentFrame(posedState, 0), 2U * kCharacterFrames);
    EXPECT_EQ(getCurrentFrame(posedState, kCharacterPaceTick),
              (2U * kCharacterFrames) + 1U);
}

TEST(CharacterTest, CurrentFrame_HoldsOneFrameWhileItStandsStill)
{
    const AnimationState posedState{
        .direction = 3, .walking = false, .startedAtTick = 0};

    EXPECT_EQ(getCurrentFrame(posedState, 0), 3U * kCharacterFrames);
    EXPECT_EQ(
        getCurrentFrame(
            posedState,
            kCharacterPaceTick * 9), 3U * kCharacterFrames);
}

TEST(CharacterTest, CharacterPixel_KeepsAPixelInsideItsOwnFrame)
{
    const auto pixelCell =
        getCharacterPixel(2, 1, antwika::geometry::GridCell{1, 3});

    EXPECT_EQ(pixelCell.column, kCharacterCellSize.width + 1);
    EXPECT_EQ(pixelCell.row, (2 * kCharacterCellSize.height) + 3);
}

TEST(CharacterTest, PaintCharacter_ColorsOnePixelOfOneFrame)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacter(
        sheet, 1, 2, antwika::geometry::GridCell{0, 0}, kRedColor);

    const auto pixelCell = getCharacterPixel(
        1, 2, antwika::geometry::GridCell{0, 0});
    const auto byteIndex =
        ((pixelCell.row * sheet.size.width) + pixelCell.column)
        * antwika::gfx::kBytesPerPixel;

    EXPECT_EQ(sheet.pixels[byteIndex], 255);
    EXPECT_EQ(sheet.pixels[byteIndex + 1], 0);
}

TEST(CharacterTest, PaintCharacterFill_FloodsAPatchUpToItsBounds)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    constexpr antwika::gfx::Color kBlueColor{
        .red = 0, .green = 0, .blue = 255, .alpha = 255};

    for (std::uint32_t row = 0;
         row < kCharacterCellSize.height;
         ++row)
    {
        paintCharacter(
            sheet,
            1,
            2,
            antwika::geometry::GridCell{.column = 5, .row = row},
            kRedColor);
    }

    paintCharacterFill(
        sheet,
        1,
        2,
        antwika::geometry::GridCell{.column = 0, .row = 0},
        kBlueColor);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            1,
            2,
            antwika::geometry::GridCell{.column = 4, .row = 20}),
        kBlueColor);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            1,
            2,
            antwika::geometry::GridCell{.column = 5, .row = 20}),
        kRedColor);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            1,
            2,
            antwika::geometry::GridCell{.column = 6, .row = 20})
            .alpha,
        0);
}

TEST(CharacterTest, PaintCharacterFill_StaysWithinItsOwnFrame)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacterFill(
        sheet,
        1,
        2,
        antwika::geometry::GridCell{.column = 0, .row = 0},
        kRedColor);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            1,
            2,
            antwika::geometry::GridCell{
                .column = kCharacterCellSize.width - 1,
                .row = kCharacterCellSize.height - 1}),
        kRedColor);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            1,
            3,
            antwika::geometry::GridCell{.column = 0, .row = 0})
            .alpha,
        0);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            2,
            2,
            antwika::geometry::GridCell{.column = 0, .row = 0})
            .alpha,
        0);
}

TEST(CharacterTest, CharacterAt_FindsTheFrameAPointFallsOn)
{
    const auto where = getCharacterPlace(kCanvasSize, 3, 2);
    const antwika::gfx::PointF middlePoint{
        where.originPoint.x + (where.size.width / 2.0F),
        where.originPoint.y + (where.size.height / 2.0F)};

    EXPECT_EQ(
        characterAt(kCanvasSize, middlePoint), (3U * kCharacterFrames) + 2U);
}

TEST(CharacterTest, CharacterAt_FindsNothingBeyondTheSheet)
{
    EXPECT_FALSE(
        characterAt(kCanvasSize, antwika::gfx::PointF{479.0F, 269.0F})
            .has_value());
}

TEST(CharacterTest, CharacterPixelAt_NamesThePixelAPressColors)
{
    const auto where = getCharacterCanvasRect(kCanvasSize);
    const auto pixel = antwika::character::characterPixelAt(
        where,
        antwika::gfx::PointF{
            where.originPoint.x + 1.0F, where.originPoint.y + 1.0F});

    ASSERT_TRUE(pixel.has_value());
    EXPECT_EQ(pixel->column, 0U);
    EXPECT_EQ(pixel->row, 0U);
}

TEST(CharacterTest, CharacterPixelAt_FindsNothingOutsideTheFrame)
{
    const auto where = getCharacterCanvasRect(kCanvasSize);

    EXPECT_FALSE(
        antwika::character::characterPixelAt(
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

TEST(
    CharacterTest,
    CharacterPaletteColor_RubsOutWithTheInkACharacterIsBlankIn)
{
    const auto paletteColor =
        getCharacterPaletteColor(antwika::tile::kPaletteColors, 0).alpha;
    const auto color =
        getCharacterPaletteColor(antwika::tile::kPaletteColors, 2);

    EXPECT_EQ(paletteColor, 0);
    EXPECT_EQ(color, antwika::tile::kPaletteColors.at(2));
}

TEST(CharacterTest, BlankCharacter_HasNothingDrawnOnItAtAll)
{
    const auto sheet = getBlankCharacter();

    for (std::size_t index = antwika::gfx::kBytesPerPixel - 1;
         index < sheet.pixels.size();
         index += antwika::gfx::kBytesPerPixel)
    {
        ASSERT_EQ(sheet.pixels[index], 0);
    }
}

TEST(CharacterTest, CharacterMesh_HangsAQuadOnWhereItsFootprintIs)
{
    const auto mesh = antwika::character::getCharacterMesh();

    ASSERT_EQ(mesh.vertices.size(), 6U);
    EXPECT_TRUE(mesh.isComplete());
    EXPECT_EQ(mesh.getTriangleCount(), 4U);

    auto lowest = mesh.vertices.front().position.y;
    auto tallest = lowest;

    for (const auto &corner : mesh.vertices)
    {
        lowest = std::min(lowest, corner.position.y);
        tallest = std::max(tallest, corner.position.y);
    }

    EXPECT_NEAR(
        lowest, -antwika::collision::kFootprintPivotY, 0.0001F);
    EXPECT_NEAR(
        tallest - lowest, antwika::character::kCharacterTall, 0.0001F);
}

TEST(CharacterTest, SpriteBillboardMatrix_StandsTheQuadOverWhereItIsPut)
{
    const antwika::gfx::Vec3 position{3.0F, 1.5F, -2.0F};
    const auto camera = antwika::gfx::Camera3D{
        antwika::gfx::Vec3{4.0F, 6.0F, 8.0F},
        antwika::gfx::Vec3{0.0F, 0.0F, 0.0F},
        antwika::gfx::Vec3{0.0F, 1.0F, 0.0F},
        antwika::gfx::Perspective{
            .fovYRadians = 1.0F,
            .aspectRatio = 1.0F,
            .nearPlane = 0.1F,
            .farPlane = 100.0F}};
    const auto view = camera.getView();
    const auto stood =
        antwika::character::getSpriteBillboardMatrix(position, view)
        * antwika::gfx::Vec4{0.0F, 0.0F, 0.0F, 1.0F};

    EXPECT_NEAR(stood.x, position.x, 0.0001F);
    EXPECT_NEAR(stood.z, position.z, 0.0001F);
    EXPECT_GE(
        stood.y,
        position.y + antwika::character::kSpriteDepthBias
            + antwika::character::kSpriteLift);
    EXPECT_LE(
        stood.y,
        position.y + antwika::character::kSpriteDepthBias
            + antwika::character::kSpriteLift
            + antwika::collision::kFootprintPivotY);
}

TEST(CharacterTest, SpriteBillboardMatrix_LaysTheQuadSquareToTheCamera)
{
    const auto camera = antwika::gfx::Camera3D{
        antwika::gfx::Vec3{4.0F, 6.0F, 8.0F},
        antwika::gfx::Vec3{0.0F, 0.0F, 0.0F},
        antwika::gfx::Vec3{0.0F, 1.0F, 0.0F},
        antwika::gfx::Perspective{
            .fovYRadians = 1.0F,
            .aspectRatio = 1.0F,
            .nearPlane = 0.1F,
            .farPlane = 100.0F}};
    const auto view = camera.getView();
    const auto billboardMatrix = antwika::character::getSpriteBillboardMatrix(
        antwika::gfx::Vec3{3.0F, 1.5F, -2.0F}, view);

    for (const auto &[cornerPoint, wantedPoint] :
         {std::pair{
              antwika::gfx::Vec4{1.0F, 0.0F, 0.0F, 0.0F},
              antwika::gfx::Vec3{1.0F, 0.0F, 0.0F}},
          std::pair{
              antwika::gfx::Vec4{0.0F, 1.0F, 0.0F, 0.0F},
              antwika::gfx::Vec3{0.0F, 1.0F, 0.0F}}})
    {
        const auto seenPoint = view * billboardMatrix * cornerPoint;

        EXPECT_NEAR(seenPoint.x, wantedPoint.x, 0.0001F);
        EXPECT_NEAR(seenPoint.y, wantedPoint.y, 0.0001F);
        EXPECT_NEAR(seenPoint.z, wantedPoint.z, 0.0001F);
    }
}

TEST(CharacterTest, FrameUvOffset_NamesEveryFrameOfTheSheet)
{
    const auto span = antwika::character::getFrameUvSize();

    EXPECT_NEAR(
        span.x, 1.0F / static_cast<float>(kCharacterFrames), 0.0001F);
    EXPECT_NEAR(
        span.y, 1.0F / static_cast<float>(kCharacterWays), 0.0001F);

    const auto fromOffset = antwika::character::getFrameUvOffset(3, 2);

    EXPECT_NEAR(fromOffset.x, 2.0F * span.x, 0.0001F);
    EXPECT_NEAR(fromOffset.y, 3.0F * span.y, 0.0001F);
}

TEST(CharacterTest, CharacterPixelPlace_LeavesOnePixelClearOfTheNext)
{
    const auto where = getCharacterCanvasRect(kCanvasSize);
    const auto overRect =
        antwika::character::getCharacterPixelPlace(
            where, antwika::geometry::GridCell{2, 3});
    const auto underRect =
        antwika::character::getCharacterPixelPlace(
            where, antwika::geometry::GridCell{2, 4});

    EXPECT_LT(
        overRect.originPoint.y + overRect.size.height, underRect.originPoint.y);
    EXPECT_NEAR(overRect.size.width, underRect.size.width, 0.0001F);
}

TEST(CharacterTest, CharacterPixelPlace_LeavesAsMuchClearAcrossAsDown)
{
    const auto where = getCharacterCanvasRect(kCanvasSize);
    const auto first = antwika::character::getCharacterPixelPlace(
        where, antwika::geometry::GridCell{1, 1});
    const auto besideRect = antwika::character::getCharacterPixelPlace(
        where, antwika::geometry::GridCell{2, 1});
    const auto belowRect = antwika::character::getCharacterPixelPlace(
        where, antwika::geometry::GridCell{1, 2});

    EXPECT_NEAR(
        besideRect.originPoint.x - (first.originPoint.x + first.size.width),
        belowRect.originPoint.y - (first.originPoint.y + first.size.height),
        0.0001F);
}

TEST(CharacterTest, CharacterPixelPlace_KeepsAPixelWhereAPressLandsIt)
{
    const auto where = getCharacterCanvasRect(kCanvasSize);
    const antwika::geometry::GridCell pixelCell{5, 7};
    const auto place =
        antwika::character::getCharacterPixelPlace(where, pixelCell);
    const antwika::gfx::PointF middlePoint{
        place.originPoint.x + (place.size.width / 2.0F),
        place.originPoint.y + (place.size.height / 2.0F)};

    EXPECT_EQ(
        antwika::character::characterPixelAt(where, middlePoint),
        pixelCell);
}

TEST(CharacterTest, CharacterPixelColor_ReadsBackWhatWasLaidDown)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacter(
        sheet, 4, 1, antwika::geometry::GridCell{2, 2}, kRedColor);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 4, 1, antwika::geometry::GridCell{2, 2}),
        kRedColor);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 4, 1, antwika::geometry::GridCell{3, 2})
            .alpha,
        0);
}

TEST(CharacterTest, CharacterMesh_LaysWhatIsUnderTheGroundLineFlat)
{
    const auto mesh = antwika::character::getCharacterMesh();

    for (const auto &corner : mesh.vertices)
    {
        if (corner.position.y > 0.0F)
        {
            continue;
        }

        EXPECT_NEAR(
            corner.position.z,
            -corner.position.y * antwika::character::kSpriteGroundSkew,
            0.0001F);
    }
}

TEST(CharacterTest, CharacterMesh_StandsWhatIsOverTheGroundLineUp)
{
    const auto mesh = antwika::character::getCharacterMesh();

    for (const auto &corner : mesh.vertices)
    {
        if (corner.position.y < 0.0F)
        {
            continue;
        }

        EXPECT_NEAR(
            corner.position.z,
            corner.position.y * antwika::character::kSpriteUprightSkew,
            0.0001F);
    }
}

TEST(CharacterTest, CharacterMesh_StandsItsFootClearOfItsOwnGround)
{
    const auto mesh = antwika::character::getCharacterMesh();
    auto lowest = mesh.vertices.front();

    for (const auto &corner : mesh.vertices)
    {
        if (corner.position.y < lowest.position.y)
        {
            lowest = corner;
        }
    }

    EXPECT_NEAR(
        lowest.position.z,
        antwika::collision::kFootprintPivotY
            * antwika::character::kSpriteGroundSkew,
        0.0001F);
    EXPECT_GT(lowest.position.z, 0.0F);
}

TEST(CharacterTest, CopiedFrom_TakesWhatLiesBetweenTwoCorners)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{2, 3}, kRedColor);

    const auto buffer = antwika::character::copiedFrom(
        sheet,
        0,
        0,
        antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{1, 2},
            .toCell = antwika::geometry::GridCell{3, 4}});

    EXPECT_EQ(buffer.size.width, 3U);
    EXPECT_EQ(buffer.size.height, 3U);
    EXPECT_EQ(buffer.pixelColors.at(4), kRedColor);
}

TEST(CharacterTest, CopiedFrom_TakesTheSameEitherWayRound)
{
    const auto sheet = getBlankCharacter();
    const antwika::geometry::GridCell oneCell{1, 2};
    const antwika::geometry::GridCell otherCell{5, 7};

    EXPECT_EQ(
        antwika::character::copiedFrom(
            sheet,
            2,
            1,
            antwika::character::PixelSelection{
                .fromCell = oneCell, .toCell = otherCell}),
        antwika::character::copiedFrom(
            sheet,
            2,
            1,
            antwika::character::PixelSelection{
                .fromCell = otherCell, .toCell = oneCell}));
}

TEST(CharacterTest, PasteInto_LaysABufferDownWhereItIsPut)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{0, 0}, kRedColor);

    const auto buffer = antwika::character::copiedFrom(
        sheet,
        0,
        0,
        antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{0, 0},
            .toCell = antwika::geometry::GridCell{1, 1}});

    antwika::character::pasteInto(
        sheet, 1, 2, antwika::geometry::GridCell{4, 5}, buffer);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 1, 2, antwika::geometry::GridCell{4, 5}),
        kRedColor);
}

TEST(CharacterTest, PasteInto_LeavesOffWhatWouldFallOutsideTheFrame)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    for (std::uint32_t row = 0; row < 2; ++row)
    {
        for (std::uint32_t column = 0; column < 2; ++column)
        {
            paintCharacter(
                sheet,
                0,
                0,
                antwika::geometry::GridCell{column, row},
                kRedColor);
        }
    }

    const auto buffer = antwika::character::copiedFrom(
        sheet,
        0,
        0,
        antwika::character::PixelSelection{
            .fromCell = antwika::geometry::GridCell{0, 0},
            .toCell = antwika::geometry::GridCell{1, 1}});

    antwika::character::pasteInto(
        sheet,
        0,
        0,
        antwika::geometry::GridCell{
            kCharacterCellSize.width - 1, kCharacterCellSize.height - 1},
        buffer);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 1, 0, antwika::geometry::GridCell{0, 0})
            .alpha,
        0);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            0,
            0,
            antwika::geometry::GridCell{
                kCharacterCellSize.width - 1,
                kCharacterCellSize.height - 1}),
        kRedColor);
}

TEST(CharacterTest, SelectionRect_HoldsBothCornersItIsMarkedBy)
{
    const auto where = getCharacterCanvasRect(kCanvasSize);
    const antwika::geometry::GridCell oneCell{2, 3};
    const antwika::geometry::GridCell otherCell{6, 8};
    const auto markedRect = antwika::character::getSelectionRect(
        where, antwika::character::PixelSelection{
            .fromCell = otherCell,
            .toCell = oneCell});
    const auto first =
        antwika::character::getCharacterPixelPlace(where, oneCell);
    const auto lastRect =
        antwika::character::getCharacterPixelPlace(where, otherCell);

    EXPECT_NEAR(markedRect.originPoint.x, first.originPoint.x, 0.0001F);
    EXPECT_NEAR(markedRect.originPoint.y, first.originPoint.y, 0.0001F);
    EXPECT_NEAR(
        markedRect.originPoint.x + markedRect.size.width,
        lastRect.originPoint.x + lastRect.size.width,
        0.0001F);
    EXPECT_NEAR(
        markedRect.originPoint.y + markedRect.size.height,
        lastRect.originPoint.y + lastRect.size.height,
        0.0001F);
}

TEST(CharacterTest, PaintCharacterLine_LeavesNoGapBetweenTwoPixels)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};

    antwika::character::paintCharacterLine(
        sheet,
        1,
        2,
        antwika::geometry::GridCell{2, 2},
        antwika::geometry::GridCell{6, 4},
        kRedColor);

    for (const auto pixel : antwika::tile::getLinePixels(
             antwika::geometry::GridCell{2, 2},
             antwika::geometry::GridCell{6, 4}))
    {
        EXPECT_EQ(
            antwika::character::getCharacterPixelColor(sheet, 1, 2, pixel),
            kRedColor);
    }
}

TEST(CharacterTest, PaintCharacterLine_ColorsOnePixelForNoLength)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::geometry::GridCell onlyCell{3, 3};

    antwika::character::paintCharacterLine(
        sheet,
        0,
        0,
        onlyCell,
        onlyCell,
        kRedColor);

    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet,
            0,
            0,
            onlyCell), kRedColor);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 0, 0, antwika::geometry::GridCell{4, 3})
            .alpha,
        0);
}

TEST(CharacterTest, SelectionOrigin_TakesTheSameCornerEitherWayRound)
{
    const antwika::geometry::GridCell oneCell{2, 5};
    const antwika::geometry::GridCell otherCell{6, 3};

    EXPECT_EQ(
        antwika::character::getSelectionOrigin(
            antwika::character::PixelSelection{
                .fromCell = oneCell,
                .toCell = otherCell}),
        (antwika::geometry::GridCell{2, 3}));
    EXPECT_EQ(
        antwika::character::getSelectionOrigin(
            antwika::character::PixelSelection{
                .fromCell = otherCell,
                .toCell = oneCell}),
        (antwika::geometry::GridCell{2, 3}));
}

TEST(CharacterTest, SelectionSize_CountsBothPixelsItIsMarkedBy)
{
    EXPECT_EQ(
        antwika::character::getSelectionSize(
            antwika::character::PixelSelection{
                .fromCell = antwika::geometry::GridCell{2, 5},
                .toCell = antwika::geometry::GridCell{6, 3}}),
        (antwika::gfx::Size{.width = 5, .height = 3}));
}

TEST(CharacterTest, SelectionSize_HoldsOnePixelForASelectionOfOne)
{
    const antwika::geometry::GridCell onlyCell{4, 4};

    EXPECT_EQ(
        antwika::character::getSelectionSize(
            antwika::character::PixelSelection{
                .fromCell = onlyCell,
                .toCell = onlyCell}),
        (antwika::gfx::Size{.width = 1, .height = 1}));
}

TEST(CharacterTest, SelectionContains_TakesEveryPixelWithinTheSelection)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{2, 3},
        .toCell = antwika::geometry::GridCell{4, 5}};

    for (std::uint32_t row = 3; row <= 5; ++row)
    {
        for (std::uint32_t column = 2; column <= 4; ++column)
        {
            EXPECT_TRUE(
                antwika::character::isSelectionContains(
                    selection,
                    antwika::geometry::GridCell{column, row}));
        }
    }
}

TEST(CharacterTest, SelectionContains_TakesNoPixelBesideTheSelection)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{2, 3},
        .toCell = antwika::geometry::GridCell{4, 5}};

    EXPECT_FALSE(
        antwika::character::isSelectionContains(
            selection, antwika::geometry::GridCell{1, 3}));
    EXPECT_FALSE(
        antwika::character::isSelectionContains(
            selection, antwika::geometry::GridCell{5, 5}));
    EXPECT_FALSE(
        antwika::character::isSelectionContains(
            selection, antwika::geometry::GridCell{3, 6}));
}

TEST(CharacterTest, MovedSelection_KeepsTheSizeItWasGiven)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{2, 3},
        .toCell = antwika::geometry::GridCell{5, 7}};
    const auto movedRect = antwika::character::getMovedSelection(selection, 3, -2);

    EXPECT_EQ(
        antwika::character::getSelectionSize(movedRect),
        antwika::character::getSelectionSize(selection));
    EXPECT_EQ(
        antwika::character::getSelectionOrigin(movedRect),
        (antwika::geometry::GridCell{5, 1}));
}

TEST(CharacterTest, MovedSelection_HoldsASelectionWithinTheFrame)
{
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{0, 0},
        .toCell = antwika::geometry::GridCell{3, 3}};
    const auto clampedLow = antwika::character::getMovedSelection(selection, -4,
        -4);
    const auto clampedHigh = antwika::character::getMovedSelection(selection, 999,
        999);

    EXPECT_EQ(
        antwika::character::getSelectionOrigin(clampedLow),
        (antwika::geometry::GridCell{0, 0}));
    EXPECT_EQ(
        antwika::character::getSelectionOrigin(clampedHigh),
        (antwika::geometry::GridCell{
            kCharacterCellSize.width - 4, kCharacterCellSize.height - 4}));
    EXPECT_EQ(
        antwika::character::getSelectionSize(clampedHigh),
        antwika::character::getSelectionSize(selection));
}

TEST(CharacterTest, CutFrom_TakesThePixelsItLeavesBlank)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{1, 1},
        .toCell = antwika::geometry::GridCell{2, 2}};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{1, 1}, kRedColor);

    const auto was = antwika::character::copiedFrom(sheet, 0, 0, selection);
    const auto cutBuffer = antwika::character::cutFrom(sheet, 0, 0, selection);

    EXPECT_EQ(cutBuffer, was);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            sheet, 0, 0, antwika::geometry::GridCell{1, 1})
            .alpha,
        0);
}

TEST(CharacterTest, CutFrom_LeavesTheFrameAsItStoodWhenLaidBack)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{3, 4},
        .toCell = antwika::geometry::GridCell{5, 6}};

    paintCharacter(
        sheet, 1, 1, antwika::geometry::GridCell{4, 5}, kRedColor);

    const auto was = sheet;
    const auto cutBuffer = antwika::character::cutFrom(sheet, 1, 1, selection);

    antwika::character::pasteInto(
        sheet, 1, 1, antwika::character::getSelectionOrigin(selection), cutBuffer);

    EXPECT_EQ(sheet.pixels, was.pixels);
}

TEST(CharacterTest, SpriteBillboardMatrix_HoldsEveryCornerOverTheGround)
{
    const antwika::gfx::Vec3 position{3.0F, 1.5F, -2.0F};
    const auto billboardMatrix = antwika::character::getSpriteBillboardMatrix(
        position, getSquaredView());

    for (const auto &corner : antwika::character::getCharacterMesh().vertices)
    {
        const auto stoodPoint =
            billboardMatrix
            * antwika::gfx::Vec4{corner.position, 1.0F};

        EXPECT_GT(stoodPoint.y, position.y);
    }
}

TEST(CharacterTest, SpriteBillboardMatrix_StandsTheBodyOverTheFootprint)
{
    const antwika::gfx::Vec3 position{3.0F, 1.5F, -2.0F};
    const auto billboardMatrix = antwika::character::getSpriteBillboardMatrix(
        position, getSquaredView());

    for (const auto &corner : antwika::character::getCharacterMesh().vertices)
    {
        const auto stoodPoint =
            billboardMatrix * antwika::gfx::Vec4{corner.position, 1.0F};

        if (corner.position.y < 0.0F)
        {
            EXPECT_GT(stoodPoint.z, position.z);
            EXPECT_NEAR(
                stoodPoint.y,
                position.y + antwika::character::kSpriteDepthBias
                    + antwika::character::kSpriteLift,
                0.0001F);

            continue;
        }

        EXPECT_NEAR(stoodPoint.z, position.z, 0.0001F);
        EXPECT_GE(
            stoodPoint.y,
            position.y + antwika::character::kSpriteDepthBias
                + antwika::character::kSpriteLift);
    }
}

TEST(CharacterTest, MirroredHorizontally_ReadsEveryRowTheOtherWayAbout)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{0, 0},
        .toCell = antwika::geometry::GridCell{2, 1}};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{0, 1}, kRedColor);

    const auto mirroredBuffer = antwika::character::getMirroredHorizontally(
        antwika::character::copiedFrom(sheet, 0, 0, selection));

    EXPECT_EQ(mirroredBuffer.size.width, 3U);
    EXPECT_EQ(mirroredBuffer.size.height, 2U);
    EXPECT_EQ(mirroredBuffer.pixelColors.at(5), kRedColor);
    EXPECT_EQ(mirroredBuffer.pixelColors.at(3).alpha, 0);
}

TEST(CharacterTest, MirroredHorizontally_GivesBackWhatItWasGivenTwiceOver)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{1, 1},
        .toCell = antwika::geometry::GridCell{5, 4}};

    for (const auto pixel :
         {antwika::geometry::GridCell{1, 1},
          antwika::geometry::GridCell{2, 3},
          antwika::geometry::GridCell{5, 4}})
    {
        paintCharacter(sheet, 1, 2, pixel, kRedColor);
    }

    const auto buffer = antwika::character::copiedFrom(sheet, 1, 2, selection);

    EXPECT_EQ(
        antwika::character::getMirroredHorizontally(
            antwika::character::getMirroredHorizontally(buffer)),
        buffer);
}

TEST(
    CharacterTest,
    MirroredHorizontally_LeavesASelectionOfOneColumnAsItWas)
{
    auto sheet = getBlankCharacter();
    constexpr antwika::gfx::Color kRedColor{
        .red = 255, .green = 0, .blue = 0, .alpha = 255};
    const antwika::character::PixelSelection selection{
        .fromCell = antwika::geometry::GridCell{3, 0},
        .toCell = antwika::geometry::GridCell{3, 2}};

    paintCharacter(
        sheet, 0, 0, antwika::geometry::GridCell{3, 1}, kRedColor);

    const auto buffer = antwika::character::copiedFrom(sheet, 0, 0, selection);

    EXPECT_EQ(antwika::character::getMirroredHorizontally(buffer), buffer);
}

TEST(CharacterTest, SpriteBillboardMatrix_KeepsThePictureOutOfTheGround)
{
    const antwika::gfx::Vec3 position{3.0F, 1.5F, -2.0F};

    for (const auto tilt :
         {0.35F, 0.7F, 0.927F, 1.2F, 1.45F})
    {
        const auto billboardMatrix =
            antwika::character::getSpriteBillboardMatrix(
                position, getViewTiltedBy(-tilt));

        for (const auto &corner :
             antwika::character::getCharacterMesh().vertices)
        {
            const auto stoodPoint =
                billboardMatrix
                * antwika::gfx::Vec4{corner.position, 1.0F};

            EXPECT_GE(stoodPoint.y, position.y);
        }
    }
}

TEST(
    CharacterTest,
    SpriteBillboardMatrix_CarriesThePictureNoHigherThanItMust)
{
    const antwika::gfx::Vec3 position{};
    const auto squareMatrix = antwika::character::getSpriteBillboardMatrix(
        position, getSquaredView());
    const auto tiltMatrix = antwika::character::getSpriteBillboardMatrix(
        position, getViewTiltedBy(-0.35F));
    const antwika::gfx::Vec4 originPoint{0.0F, 0.0F, 0.0F, 1.0F};

    EXPECT_NEAR(
        (squareMatrix * originPoint).y,
        antwika::character::kSpriteDepthBias
            + antwika::character::kSpriteLift,
        0.0001F);
    EXPECT_GT((tiltMatrix * originPoint).y, (squareMatrix * originPoint).y);
    EXPECT_LT(
        (tiltMatrix * originPoint).y,
        antwika::character::kSpriteDepthBias
            + antwika::character::kSpriteLift
            + antwika::collision::kFootprintPivotY);
}

TEST(CharacterTest, HeadTopOf_StandsAtTheTopOfACharactersPicture)
{
    const antwika::component::Position stoodPosition{
        .x = 2.0F, .y = 5.0F, .z = -1.0F};
    const auto crown = headTopOf(stoodPosition);

    EXPECT_NEAR(crown.x, stoodPosition.x, 1e-4F);
    EXPECT_NEAR(crown.z, stoodPosition.z, 1e-4F);
    EXPECT_NEAR(
        crown.y,
        stoodPosition.y - antwika::collision::kFootprintPivotY
            + antwika::character::kCharacterTall
            + antwika::character::kSpriteLift,
        1e-4F);
    EXPECT_GT(crown.y, stoodPosition.y);
}
