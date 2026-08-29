#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <utility>

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/loadout/Descriptors.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/WorldCamera.hpp"
#include "antwika/editor/fakes/CanvasPoints.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/fakes/FakeEditSteps.hpp"
#include "antwika/editor/fakes/FakeNotices.hpp"
#include "antwika/editor/fakes/ViewHarness.hpp"
#include "antwika/editor/ui/AtlasSheetsView.hpp"
#include "antwika/editor/ui/AtlasView.hpp"
#include "antwika/editor/ui/CharacterView.hpp"
#include "antwika/editor/ui/ColorPicker.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"
#include "antwika/editor/ui/WorldView.hpp"

using antwika::editor::AtlasSheetsView;
using antwika::editor::CharacterSheetView;
using antwika::editor::Paint;
using antwika::editor::Tool;
using antwika::editor::WorldView;
using antwika::editor::fakes::FakeEditSteps;
using antwika::editor::fakes::FakeNotices;
using antwika::editor::fakes::ViewHarness;
using antwika::editor::fakes::getMiddleOf;
using antwika::editor::fakes::getPointerPositionAt;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    struct WorldAim final
    {
        antwika::gfx::PointF canvasPoint;

        antwika::voxel::VoxelPosition cellPosition;
    };

    [[nodiscard]] std::optional<WorldAim> getWorldAim(
        const ViewHarness &harness, const std::int32_t level)
    {
        const auto camera = antwika::editor::getWorldCamera(
            harness.play, harness.cameraRig);
        const auto rotation = antwika::editor::getWorldRotation(harness.play);

        for (std::uint32_t y = 10; y < antwika::camera::kCanvasSize.height;
             y += 10)
        {
            for (std::uint32_t x = 10; x < antwika::camera::kCanvasSize.width;
                 x += 10)
            {
                const antwika::gfx::PointF point{
                    static_cast<float>(x), static_cast<float>(y)};
                const auto cell = antwika::voxelmap::getCellUnder(
                    camera,
                    rotation,
                    antwika::camera::kCanvasSize,
                    point,
                    antwika::voxel::getCubeTop(level));

                if (cell.has_value())
                {
                    return WorldAim{.canvasPoint = point, .cellPosition = *cell};
                }
            }
        }

        return std::nullopt;
    }

    class UndoDisciplineTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        FakeEditSteps steps;
        FakeNotices notices;
        ViewHarness harness{logger, steps, notices};
    };

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    [[nodiscard]] std::size_t playerIndexOf(
        const antwika::editor::fakes::EditorProbe &probe)
    {
        for (std::size_t index = 0;
             index < probe.document.map.characters.size();
             ++index)
        {
            if (probe.document.map.characters.at(index).player)
            {
                return index;
            }
        }

        return 0;
    }

    [[nodiscard]] std::size_t componentSlotOf(
        const antwika::map::Character &character,
        const std::string_view name)
    {
        for (std::size_t slot = 0; slot < character.components.size();
             ++slot)
        {
            if (character.components.at(slot) == name)
            {
                return slot;
            }
        }

        return character.components.size();
    }

    [[nodiscard]] std::size_t registryIndexOf(
        const std::string_view name)
    {
        const auto rows = antwika::loadout::getComponentRows();

        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            if (rows[index].name == name)
            {
                return index;
            }
        }

        return rows.size();
    }

    [[nodiscard]] std::uint16_t playedFoodOf(
        antwika::ecs::World &world, const std::size_t characterIndex)
    {
        for (const auto standing :
             world.view<antwika::component::CharacterIndex>())
        {
            if (world.get<antwika::component::CharacterIndex>(standing)
                    .index
                == characterIndex)
            {
                return world.get<antwika::component::Health>(standing)
                    .food;
            }
        }

        return 0;
    }

    class ValueUndoTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

    using RecordUndoTest = ValueUndoTest;

}

TEST_F(RecordUndoTest, MarkerFieldEnter_PushesOneSnapshotForTheMove)
{
    constexpr antwika::voxel::VoxelPosition markerPosition{
        .x = 1, .y = 2, .z = 3};
    auto &cells = probe.document.map.markers.positionsOf(
        antwika::map::Marker::Checkpoint);

    probe.preferences().tool = Tool::Checkpoint;
    cells.push_back(markerPosition);
    probe.pressMarker(markerPosition, MouseButton::Left);

    const auto countBefore = probe.document.getUndoCount();
    const auto fieldWidget = antwika::editor::getMarkerFieldWidget(0);

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{.activatedWidget = fieldWidget}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);

    probe.carryEdit(fieldWidget, "5");

    EXPECT_TRUE(
        probe.consumeTextInput(
            antwika::input::KeyPressed{
                .key = antwika::input::Key::Enter}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);
    EXPECT_EQ(
        cells.back(),
        (antwika::voxel::VoxelPosition{.x = 11, .y = 2, .z = 3}));

    probe.undo();

    EXPECT_EQ(cells.back(), markerPosition);
}

TEST_F(RecordUndoTest, MarkerRemoveButton_PushesOneSnapshotForTheRemoval)
{
    constexpr antwika::voxel::VoxelPosition markerPosition{
        .x = 1, .y = 2, .z = 3};
    auto &cells = probe.document.map.markers.positionsOf(
        antwika::map::Marker::Checkpoint);

    probe.preferences().tool = Tool::Checkpoint;
    cells.push_back(markerPosition);
    probe.pressMarker(markerPosition, MouseButton::Left);

    const auto countBefore = probe.document.getUndoCount();

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::kMarkerRemoveWidget}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);
    EXPECT_TRUE(cells.empty());
    EXPECT_FALSE(probe.markerPick().marker.has_value());

    probe.undo();

    EXPECT_EQ(cells.size(), 1U);
    EXPECT_EQ(cells.back(), markerPosition);
}

TEST_F(ValueUndoTest, FieldFocusTypeEnter_PushesOneSnapshotAndUndoes)
{
    const auto playerIndex = playerIndexOf(probe);
    auto &character = probe.document.map.characters.at(playerIndex);
    const auto healthSlot =
        componentSlotOf(character, "component::Health");

    ASSERT_LT(healthSlot, character.components.size());

    probe.characterTool().choose(playerIndex);

    const auto countBefore = probe.document.getUndoCount();

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentHeadWidget(
                        healthSlot)}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);

    const auto foodWidget =
        antwika::editor::getComponentFieldWidget(healthSlot, 0);

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{.activatedWidget = foodWidget}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);

    probe.carryEdit(foodWidget, "7");

    EXPECT_TRUE(
        probe.consumeTextInput(
            antwika::input::KeyPressed{
                .key = antwika::input::Key::Enter}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);
    EXPECT_EQ(
        std::get<antwika::component::Health>(
            character.componentValues.at("component::Health"))
            .food,
        7U);
    EXPECT_EQ(playedFoodOf(probe.playedWorld(), playerIndex), 7U);

    probe.undo();

    EXPECT_FALSE(
        probe.document.map.characters.at(playerIndex)
            .componentValues.contains("component::Health"));
    EXPECT_EQ(
        playedFoodOf(probe.playedWorld(), playerIndex),
        antwika::component::kFullHealth);
}

TEST_F(ValueUndoTest, AddComponent_PushesOneSnapshot)
{
    const auto playerIndex = playerIndexOf(probe);
    const auto speakerIndex = registryIndexOf("component::Speaker");

    ASSERT_LT(
        speakerIndex, antwika::loadout::getComponentRows().size());

    probe.characterTool().choose(playerIndex);

    const auto countBefore = probe.document.getUndoCount();

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::kComponentAddOpenWidget}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore);
    EXPECT_TRUE(probe.characterTool().isAddListOpen());

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentAddWidget(
                        speakerIndex)}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);
    EXPECT_FALSE(probe.characterTool().isAddListOpen());

    const auto &character =
        probe.document.map.characters.at(playerIndex);

    EXPECT_EQ(
        componentSlotOf(character, "component::Speaker"),
        character.components.size() - 1);
    EXPECT_TRUE(character.componentValues.contains("component::Speaker"));
}

TEST_F(ValueUndoTest, DropComponent_PushesOneSnapshot)
{
    const auto playerIndex = playerIndexOf(probe);
    auto &character = probe.document.map.characters.at(playerIndex);
    const auto healthSlot =
        componentSlotOf(character, "component::Health");

    ASSERT_LT(healthSlot, character.components.size());

    probe.characterTool().choose(playerIndex);

    const auto sizeBefore = character.components.size();
    const auto countBefore = probe.document.getUndoCount();

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentDropWidget(
                        healthSlot)}));
    EXPECT_EQ(probe.document.getUndoCount(), countBefore + 1);
    EXPECT_EQ(character.components.size(), sizeBefore - 1);
    EXPECT_EQ(
        componentSlotOf(character, "component::Health"),
        character.components.size());
    EXPECT_FALSE(character.componentValues.contains("component::Health"));
}

TEST_F(UndoDisciplineTest, CharacterPress_PushesOneSnapshotBeforeThePaint)
{
    const antwika::geometry::GridCell pixelCell{4, 14};
    const auto canvasRect = antwika::editor::getCharacterCanvasRect(
        antwika::editor::getCharacterDrawBounds(
            antwika::camera::kCanvasSize,
            antwika::editor::kRightPanelWidth));
    const auto pressPoint = getMiddleOf(
        antwika::character::getCharacterPixelPlace(canvasRect, pixelCell));

    harness.inkPicker.activeInk = 2;

    auto alphaAtPush = std::optional<std::uint8_t>{};

    steps.pushProbe = [this, pixelCell, &alphaAtPush]
    {
        alphaAtPush = antwika::character::getCharacterPixelColor(
                          harness.characterView.getSheet(), 0, 0, pixelCell)
                          .alpha;
    };

    const auto viewContext = harness.contextNow();
    const auto consumedPress = harness.characterView.consumePress(
        viewContext,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = getPointerPositionAt(
                harness.viewportRenderer.getViewport(), pressPoint)});

    EXPECT_TRUE(consumedPress);
    EXPECT_EQ(steps.pushCount, 1U);
    EXPECT_EQ(alphaAtPush, 0U);
    EXPECT_EQ(
        antwika::character::getCharacterPixelColor(
            harness.characterView.getSheet(), 0, 0, pixelCell),
        harness.document.map.paletteColors.at(2));
}

TEST_F(UndoDisciplineTest, AtlasPress_PushesOneSnapshotBeforeThePaint)
{
    const antwika::tilemap::Tile tile{
        .atlas = antwika::tilemap::Atlas::Wall, .index = 0};
    const auto tileRect = antwika::editor::getInspectedTileRect(
        harness.sheetView.getFrameRect(), tile);
    const auto pressPoint = getMiddleOf(tileRect);

    ASSERT_TRUE(
        antwika::tile::pixelAt(tile, tileRect, pressPoint).has_value());

    harness.stroke.selectedTile = tile;
    harness.inkPicker.activeInk = 2;

    const auto sheetBefore =
        harness.atlasSheets.sheet(antwika::tilemap::Atlas::Wall).pixels;
    auto sheetAtPush = decltype(sheetBefore){};

    steps.pushProbe = [this, &sheetAtPush]
    {
        sheetAtPush =
            harness.atlasSheets.sheet(antwika::tilemap::Atlas::Wall).pixels;
    };

    AtlasSheetsView view;
    const auto viewContext = harness.contextNow();
    const auto consumedPress = view.consumePress(
        viewContext,
        PointerButtonPressed{
            .button = MouseButton::Left,
            .position = getPointerPositionAt(
                harness.viewportRenderer.getViewport(), pressPoint)});

    EXPECT_TRUE(consumedPress);
    EXPECT_EQ(steps.pushCount, 1U);
    EXPECT_EQ(sheetAtPush, sheetBefore);
    EXPECT_NE(
        harness.atlasSheets.sheet(antwika::tilemap::Atlas::Wall).pixels,
        sheetBefore);
}

TEST_F(UndoDisciplineTest, WorldShape_PushesOneSnapshotBeforeTheBlocksLand)
{
    WorldView view;
    const auto aim = getWorldAim(harness, view.worldEdit().getEditLevel());

    ASSERT_TRUE(aim.has_value());

    harness.preferences.tool = Tool::Brush;
    harness.preferences.paint = Paint::Rect;

    auto voxelCountAtPush = std::optional<std::size_t>{};

    steps.pushProbe = [this, &voxelCountAtPush]
    { voxelCountAtPush = harness.document.map.voxels.size(); };

    const auto viewContext = harness.contextNow();
    const auto claimedShape = view.beginShape(
        viewContext, aim->cellPosition, MouseButton::Left);

    EXPECT_TRUE(claimedShape);
    EXPECT_EQ(steps.pushCount, 0U);

    harness.pointer.pointerOnCanvas = aim->canvasPoint;
    view.finishShape(viewContext, MouseButton::Left);

    EXPECT_EQ(steps.pushCount, 1U);
    EXPECT_EQ(voxelCountAtPush, 0U);
    EXPECT_FALSE(harness.document.map.voxels.empty());
}

TEST_F(UndoDisciplineTest, StampPress_PushesOneSnapshotOnlyWhenTheStampLands)
{
    WorldView view;
    const auto aim = getWorldAim(harness, view.worldEdit().getEditLevel());

    ASSERT_TRUE(aim.has_value());

    harness.document.map.voxels = antwika::voxel::withBlockAt(
        harness.document.map.voxels,
        aim->cellPosition,
        antwika::voxel::Kind::Normal,
        antwika::voxel::Facing::Any);

    const auto voxelsBefore = harness.document.map.voxels;
    auto voxelsAtPush = std::optional<antwika::voxel::Voxels>{};

    steps.pushProbe = [this, &voxelsAtPush]
    { voxelsAtPush = harness.document.map.voxels; };

    const auto viewContext = harness.contextNow();

    view.pressStamp(viewContext, aim->cellPosition, MouseButton::Left);

    EXPECT_EQ(steps.pushCount, 0U);

    harness.pointer.pointerOnCanvas = aim->canvasPoint;
    view.finishStamp(viewContext, MouseButton::Left);

    EXPECT_EQ(steps.pushCount, 0U);

    const antwika::voxel::VoxelPosition landingPosition{
        .x = aim->cellPosition.x + 8, .y = aim->cellPosition.y, .z = aim->cellPosition.z};

    view.pressStamp(viewContext, landingPosition, MouseButton::Left);

    EXPECT_EQ(steps.pushCount, 1U);
    EXPECT_EQ(voxelsAtPush, voxelsBefore);
    EXPECT_GT(harness.document.map.voxels.size(), voxelsBefore.size());
}

TEST_F(UndoDisciplineTest, LampGrab_PushesOneSnapshotWhenALampIsTaken)
{
    const antwika::voxel::VoxelPosition lampPosition{.x = 1, .y = 2, .z = 3};

    harness.document.map.lamps.push_back(
        antwika::light::Lamp{.position = lampPosition});

    WorldView view;
    const auto viewContext = harness.contextNow();

    EXPECT_FALSE(
        view.beginLampCarry(
            viewContext,
            antwika::voxel::VoxelPosition{.x = 9, .y = 9, .z = 9}));
    EXPECT_EQ(steps.pushCount, 0U);

    EXPECT_TRUE(view.beginLampCarry(viewContext, lampPosition));
    EXPECT_EQ(steps.pushCount, 1U);
}

TEST_F(UndoDisciplineTest, InkPanelAddInk_PushesOneSnapshotBeforeTheNewInk)
{
    antwika::editor::InkPanel panel(
        harness.document,
        harness.atlasSheets,
        harness.characterView,
        harness.characterSkins,
        harness.viewportRenderer,
        steps);

    const auto inksBefore = harness.document.map.paletteColors.size();
    auto inksAtPush = std::optional<std::size_t>{};

    steps.pushProbe = [this, &inksAtPush]
    { inksAtPush = harness.document.map.paletteColors.size(); };

    const auto consumedPress = panel.consumePaletteWidgets(
        antwika::ui::Interactions{
            .activatedWidget = antwika::editor::kAddInkWidget},
        harness.pointer,
        0);

    EXPECT_TRUE(consumedPress);
    EXPECT_EQ(steps.pushCount, 1U);
    EXPECT_EQ(inksAtPush, inksBefore);
    EXPECT_EQ(
        harness.document.map.paletteColors.size(), inksBefore + 1);
}

TEST_F(UndoDisciplineTest, InkPickerDrag_PushesOneSnapshotForTheWholeDrag)
{
    antwika::editor::InkPanel panel(
        harness.document,
        harness.atlasSheets,
        harness.characterView,
        harness.characterSkins,
        harness.viewportRenderer,
        steps);

    panel.inkPicker.editingInk = 1;

    auto pickPoint = std::optional<antwika::gfx::PointF>{};

    for (std::uint32_t y = 1;
         y < antwika::camera::kCanvasSize.height && !pickPoint.has_value();
         y += 2)
    {
        for (std::uint32_t x = 1; x < antwika::camera::kCanvasSize.width;
             x += 2)
        {
            const antwika::gfx::PointF point{
                static_cast<float>(x), static_cast<float>(y)};

            if (antwika::editor::getColorAtPoint(
                    antwika::camera::kCanvasSize,
                    antwika::editor::kRightPanelWidth,
                    panel.inkPicker.pickerHsv,
                    point)
                    .has_value())
            {
                pickPoint = point;

                break;
            }
        }
    }

    ASSERT_TRUE(pickPoint.has_value());

    const auto colorBefore = harness.document.map.paletteColors.at(1);
    auto colorAtPush = std::optional<antwika::gfx::Color>{};

    steps.pushProbe = [this, &colorAtPush]
    { colorAtPush = harness.document.map.paletteColors.at(1); };

    const PointerButtonPressed downPressed{
        .button = MouseButton::Left,
        .position = getPointerPositionAt(
            harness.viewportRenderer.getViewport(), *pickPoint)};

    EXPECT_TRUE(panel.consumePickerPress(
            downPressed, harness.pointer, antwika::editor::kRightPanelWidth));
    EXPECT_TRUE(panel.consumePickerPress(
            downPressed, harness.pointer, antwika::editor::kRightPanelWidth));
    EXPECT_EQ(steps.pushCount, 1U);
    EXPECT_EQ(colorAtPush, colorBefore);
}

TEST_F(UndoDisciplineTest, PickerPress_FollowsThePickerUnderAWiderRail)
{
    constexpr auto kWideRail = antwika::editor::kRightPanelWidth * 2.0F;

    const auto widePlace = antwika::editor::getPickerPlace(
        antwika::camera::kCanvasSize, kWideRail);
    const antwika::gfx::PointF middlePoint{
        widePlace.originPoint.x + (widePlace.size.width / 2.0F),
        widePlace.originPoint.y + (widePlace.size.height / 2.0F)};
    const PointerButtonPressed downPressed{
        .button = MouseButton::Left,
        .position = getPointerPositionAt(
            harness.viewportRenderer.getViewport(), middlePoint)};

    const auto colorAfterPressAt = [this, &downPressed](const float railWidth)
    {
        antwika::editor::InkPanel panel(
            harness.document,
            harness.atlasSheets,
            harness.characterView,
            harness.characterSkins,
            harness.viewportRenderer,
            steps);

        panel.inkPicker.editingInk = 1;
        harness.document.map.paletteColors.at(1) = antwika::gfx::Color{};

        static_cast<void>(
            panel.consumePickerPress(downPressed, harness.pointer, railWidth));

        return harness.document.map.paletteColors.at(1);
    };

    EXPECT_NE(colorAfterPressAt(kWideRail), (antwika::gfx::Color{}));
    EXPECT_EQ(
        colorAfterPressAt(antwika::editor::kRightPanelWidth),
        (antwika::gfx::Color{}));
}

TEST_F(UndoDisciplineTest, PickerPress_KeepsTheInkOpenOnAWiderRailsPadding)
{
    constexpr auto kWideRail = antwika::editor::kRightPanelWidth * 2.0F;

    const auto widePlace = antwika::editor::getPickerPlace(
        antwika::camera::kCanvasSize, kWideRail);
    const antwika::gfx::PointF paddingPoint{
        widePlace.originPoint.x + 1.0F, widePlace.originPoint.y + 1.0F};
    const PointerButtonPressed downPressed{
        .button = MouseButton::Left,
        .position = getPointerPositionAt(
            harness.viewportRenderer.getViewport(), paddingPoint)};

    const auto inkAfterPressAt = [this, &downPressed](const float railWidth)
    {
        antwika::editor::InkPanel panel(
            harness.document,
            harness.atlasSheets,
            harness.characterView,
            harness.characterSkins,
            harness.viewportRenderer,
            steps);

        panel.inkPicker.editingInk = 1;

        static_cast<void>(
            panel.consumePickerPress(downPressed, harness.pointer, railWidth));

        return panel.inkPicker.editingInk;
    };

    EXPECT_TRUE(inkAfterPressAt(kWideRail).has_value());
    EXPECT_FALSE(
        inkAfterPressAt(antwika::editor::kRightPanelWidth).has_value());
}
