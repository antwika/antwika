#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/app/FramePacing.hpp>
#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/component/Health.hpp>
#include <antwika/component/CharacterIndex.hpp>
#include <antwika/gameplay/ComponentNames.hpp>
#include <antwika/gfx/NullBackend.hpp>
#include <antwika/gfx/Viewport.hpp>
#include <antwika/gfx/fakes/FakeClosingBackend.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/input/fakes/FakeScriptedInputBackend.hpp>
#include <antwika/loadout/Descriptors.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/map/MapFile.hpp>
#include <antwika/testing/ScratchDirectory.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/Voxel.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/CanvasPoints.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/CharacterView.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

using antwika::editor::Editor;
using antwika::editor::View;
using antwika::editor::fakes::getMiddleOf;
using antwika::editor::fakes::getPointerPositionAt;
using antwika::gfx::NullBackend;
using antwika::gfx::fakes::FakeClosingBackend;
using antwika::input::InputEvent;
using antwika::input::Key;
using antwika::input::KeyModifiers;
using antwika::input::KeyPressed;
using antwika::input::KeyReleased;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::input::PointerMoved;
using antwika::input::fakes::FakeScriptedInputBackend;
using antwika::log::mocks::MockLogger;
using antwika::testing::ScratchDirectory;
using ::testing::NiceMock;

namespace
{

    constexpr std::string_view kMissingMapPath =
        "assets/maps/no-such-map.json";

    [[nodiscard]] antwika::input::Position getWindowPointAt(
        const antwika::gfx::PointF canvasPoint)
    {
        const auto viewport = antwika::gfx::viewportFor(
            antwika::app::kDefaultWindowSize, antwika::camera::kCanvasSize);

        return getPointerPositionAt(viewport, canvasPoint);
    }

    [[nodiscard]] std::vector<InputEvent> getLeftClickAt(
        const antwika::input::Position position,
        const KeyModifiers modifiers = {})
    {
        return {
            InputEvent{PointerMoved{.position = position}},
            InputEvent{PointerButtonPressed{
                .button = MouseButton::Left,
                .position = position,
                .modifiers = modifiers}}};
    }

    class ScriptedEditorTest : public ::testing::Test
    {
    protected:
        [[nodiscard]] antwika::gfx::RectF characterDrawRectOf()
        {
            antwika::input::NullInputBackend restingInputs{logger};
            Editor editor(
                logger,
                nullBackend,
                restingInputs,
                std::string(kMissingMapPath));
            antwika::editor::fakes::EditorProbe probe{editor};

            probe.viewChoice().activeView = View::Character;
            probe.pumpFrame();

            return probe.sheetView().canvasRect.value_or(
                antwika::editor::getCharacterDrawBounds(
                    antwika::camera::kCanvasSize,
                    antwika::editor::kRightPanelWidth));
        }

        NiceMock<MockLogger> logger;
        NullBackend nullBackend{logger};
    };

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

}

TEST_F(ScriptedEditorTest, Run_ViewChordShowsTheCharacterSheet)
{
    FakeScriptedInputBackend inputs(
        {{InputEvent{KeyPressed{.key = Key::Digit3}}}});
    FakeClosingBackend backend(
        nullBackend, [&inputs] { return inputs.isSpent(); });
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));

    EXPECT_EQ(editor.getActiveView(), View::World);

    editor.run();

    EXPECT_EQ(editor.getActiveView(), View::Character);
}

TEST_F(ScriptedEditorTest, Run_ViewChordShowsTheGizmoSheet)
{
    FakeScriptedInputBackend inputs(
        {{InputEvent{KeyPressed{.key = Key::Digit6}}}});
    FakeClosingBackend backend(
        nullBackend, [&inputs] { return inputs.isSpent(); });
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));

    editor.run();

    EXPECT_EQ(editor.getActiveView(), View::Gizmos);
}

TEST_F(ScriptedEditorTest, Run_SaveChordWritesTheMapWhereItStarted)
{
    const ScratchDirectory scratch("editor-scripted-save");
    const auto mapPath = scratch.pathIn("scripted.json");

    FakeScriptedInputBackend inputs(
        {{InputEvent{KeyPressed{
            .key = Key::S,
            .modifiers = KeyModifiers{.control = true}}}}});
    FakeClosingBackend backend(
        nullBackend, [&inputs] { return inputs.isSpent(); });
    Editor editor(logger, backend, inputs, mapPath);

    editor.run();

    ASSERT_TRUE(std::filesystem::exists(mapPath));

    const auto savedMap = antwika::map::getLoadMap(mapPath);

    EXPECT_EQ(
        savedMap.voxels,
        antwika::voxel::getExpandCubesToVoxels(
            antwika::voxelmap::getDemoCells()));
}

TEST_F(ScriptedEditorTest, Run_PlayChordWalksThePlayerAndEscapeComesBack)
{
    std::vector<std::vector<InputEvent>> eventScript;

    eventScript.push_back({InputEvent{KeyPressed{.key = Key::F5}}});
    eventScript.push_back({InputEvent{KeyPressed{.key = Key::D}}});

    for (std::size_t heldTick = 0; heldTick < 150; ++heldTick)
    {
        eventScript.emplace_back();
    }

    eventScript.push_back({InputEvent{KeyReleased{.key = Key::D}}});
    eventScript.push_back({InputEvent{KeyPressed{.key = Key::Escape}}});

    FakeScriptedInputBackend inputs(
        std::move(eventScript), std::chrono::milliseconds{2});
    FakeClosingBackend backend(
        nullBackend, [&inputs] { return inputs.isSpent(); });
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));

    const auto stoodBefore = editor.playerStandsAt();

    editor.run();

    const auto stoodAfter = editor.playerStandsAt();
    const auto wentFar =
        std::abs(stoodAfter.x - stoodBefore.x)
        + std::abs(stoodAfter.z - stoodBefore.z);

    EXPECT_FALSE(editor.isPlaying());
    EXPECT_GT(wentFar, 0.05F);
}

TEST_F(ScriptedEditorTest, Run_FrameClickChoosesTheFrameAPointFallsOn)
{
    antwika::input::NullInputBackend restingInputs{logger};
    Editor editor(
        logger, nullBackend, restingInputs, std::string(kMissingMapPath));
    antwika::editor::fakes::EditorProbe probe{editor};

    probe.viewChoice().activeView = antwika::editor::View::Character;
    probe.pumpFrame();

    const auto sheetRect = probe.sheetView().sheetRect.value_or(
        antwika::editor::getCharacterSheetBounds(
            antwika::camera::kCanvasSize));
    const auto framePoint = getWindowPointAt(
        getMiddleOf(antwika::editor::getCharacterPlace(sheetRect, 3, 2)));

    probe.pointerPressed(
        PointerButtonPressed{
            .button = MouseButton::Left, .position = framePoint});
    probe.pointerReleased(
        PointerButtonReleased{
            .button = MouseButton::Left, .position = framePoint});

    EXPECT_EQ(
        editor.getCharacterView().getMark().selectedFrame,
        (3U * antwika::character::kCharacterFrames) + 2U);
}

TEST_F(ScriptedEditorTest, ComponentValueEdit_ReachesTheDocumentAndTheWorld)
{
    antwika::input::NullInputBackend restingInputs{logger};
    Editor editor(
        logger, nullBackend, restingInputs,
        std::string(kMissingMapPath));
    antwika::editor::fakes::EditorProbe probe{editor};

    const auto characterIndex = probe.document.map.characters.size();

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::getToolWidget(
                    antwika::editor::ToolButton::Character)}));
    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget = antwika::editor::kAddCharacterWidget}));

    const auto characterNames =
        antwika::gameplay::getCharacterComponentNames();
    const auto &characterComponents =
        probe.document.map.characters.at(characterIndex).components;

    ASSERT_EQ(characterComponents.size(), characterNames.size());

    for (std::size_t place = 0; place < characterComponents.size();
         ++place)
    {
        EXPECT_EQ(characterComponents.at(place), characterNames[place]);
    }

    const auto heldSlot = componentSlotOf(
        probe.document.map.characters.at(characterIndex),
        "component::Health");

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentDropWidget(
                        heldSlot)}));
    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::kComponentAddOpenWidget}));
    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentAddWidget(
                        registryIndexOf("component::Health"))}));

    const auto healthSlot = componentSlotOf(
        probe.document.map.characters.at(characterIndex),
        "component::Health");

    ASSERT_LT(
        healthSlot,
        probe.document.map.characters.at(characterIndex)
            .components.size());

    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentHeadWidget(
                        healthSlot)}));
    EXPECT_TRUE(
        probe.consumeWidgets(
            antwika::ui::Interactions{
                .activatedWidget =
                    antwika::editor::getComponentFieldWidget(
                        healthSlot, 0)}));

    for (const auto key :
         {Key::Backspace, Key::Backspace, Key::Backspace,
          Key::Digit7})
    {
        probe.keyPressed(KeyPressed{.key = key});
        probe.pumpFrame();
    }

    EXPECT_EQ(probe.characterTool().getPendingValueText(), "7");

    probe.keyPressed(KeyPressed{.key = Key::Enter});

    EXPECT_EQ(
        std::get<antwika::component::Health>(
            probe.document.map.characters.at(characterIndex)
                .componentValues.at("component::Health"))
            .food,
        7U);
    EXPECT_EQ(playedFoodOf(probe.playedWorld(), characterIndex), 7U);

    probe.undo();

    EXPECT_EQ(
        std::get<antwika::component::Health>(
            probe.document.map.characters.at(characterIndex)
                .componentValues.at("component::Health"))
            .food,
        antwika::component::kFullHealth);
    EXPECT_EQ(
        playedFoodOf(probe.playedWorld(), characterIndex),
        antwika::component::kFullHealth);
}

TEST_F(ScriptedEditorTest, Run_ShiftDragMarksAPixelSelection)
{
    const auto canvasRect =
        antwika::editor::getCharacterCanvasRect(characterDrawRectOf());
    const auto fromPoint = getWindowPointAt(
        getMiddleOf(
            antwika::character::getCharacterPixelPlace(
                canvasRect, antwika::geometry::GridCell{4, 14})));
    const auto toPoint = getWindowPointAt(
        getMiddleOf(
            antwika::character::getCharacterPixelPlace(
                canvasRect, antwika::geometry::GridCell{9, 18})));

    std::vector<std::vector<InputEvent>> eventScript;

    eventScript.push_back({InputEvent{KeyPressed{.key = Key::Digit3}}});
    eventScript.push_back(
        {InputEvent{KeyPressed{
            .key = Key::LeftShift,
            .modifiers = KeyModifiers{.shift = true}}}});
    eventScript.push_back(
        getLeftClickAt(fromPoint, KeyModifiers{.shift = true}));
    eventScript.push_back({InputEvent{PointerMoved{.position = toPoint}}});
    eventScript.push_back(
        {InputEvent{PointerButtonReleased{
            .button = MouseButton::Left, .position = toPoint}},
         InputEvent{KeyReleased{.key = Key::LeftShift}}});

    FakeScriptedInputBackend inputs(std::move(eventScript));
    FakeClosingBackend backend(
        nullBackend, [&inputs] { return inputs.isSpent(); });
    Editor editor(logger, backend, inputs, std::string(kMissingMapPath));

    editor.run();

    const auto &mark = editor.getCharacterView().getMark();

    ASSERT_TRUE(mark.selection.has_value());
    EXPECT_FALSE(mark.selecting);
    EXPECT_EQ(mark.selection->fromCell, (antwika::geometry::GridCell{4, 14}));
    EXPECT_EQ(mark.selection->toCell, (antwika::geometry::GridCell{9, 18}));
}
