#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <memory>
#include <string_view>
#include <vector>

#include <antwika/component/Position.hpp>
#include <antwika/ecs/OpenPhase.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IMesh.hpp>
#include <antwika/gfx/IRenderTarget.hpp>
#include <antwika/gfx/IShader.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/ShaderSource.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/gfx/fakes/FakeSizedTarget.hpp>
#include <antwika/gfx/mocks/MockMesh.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockShader.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/render/ScenePass.hpp>
#include <antwika/render/WorldShader.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/Preferences.hpp"
#include "antwika/editor/fakes/FakeEditSteps.hpp"
#include "antwika/editor/fakes/FakeNotices.hpp"
#include "antwika/editor/fakes/ViewHarness.hpp"
#include "antwika/editor/ui/EditorLook.hpp"
#include "antwika/editor/ui/GizmoSheet.hpp"
#include "antwika/editor/ui/IconSheet.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"
#include "antwika/editor/ui/WorldView.hpp"
#include "antwika/editor/view/ViewContext.hpp"

using antwika::editor::Tool;
using antwika::editor::ViewContext;
using antwika::editor::WorldView;
using antwika::editor::fakes::FakeEditSteps;
using antwika::editor::fakes::FakeNotices;
using antwika::editor::fakes::ViewHarness;
using antwika::gfx::Color;
using antwika::gfx::PointF;
using antwika::gfx::ShaderSource;
using antwika::gfx::ViewportRenderer;
using antwika::gfx::fakes::FakeSizedTarget;
using antwika::gfx::mocks::MockMesh;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockShader;
using antwika::log::mocks::MockLogger;
using ::testing::NiceMock;

namespace
{

    struct StatusCase final
    {
        Tool tool;

        std::string_view text;
    };

    constexpr std::array kStatusCases{
        StatusCase{
            Tool::Select,
            "1 world - lmb picks an entity - drag moves it - "
            "rmb lets go"},
        StatusCase{
            Tool::Brush,
            "1 world - lmb adds - rmb takes - f5 plays"},
        StatusCase{
            Tool::Picker,
            "1 world - lmb picks a tile - rmb takes"},
        StatusCase{
            Tool::Lamp,
            "1 world - lmb sets a lamp of the ink chosen - rmb takes"},
        StatusCase{
            Tool::Start,
            "1 world - lmb sets the start cube - rmb takes it"},
        StatusCase{
            Tool::Exit,
            "1 world - lmb sets the exit cube - rmb takes it"},
        StatusCase{
            Tool::Stamp,
            "1 world - drag copies cubes - lmb sets them down - rmb "
            "drops them"},
        StatusCase{
            Tool::Character,
            "1 world - lmb stands the chosen character here, again "
            "adds a stop - rmb takes it away"},
        StatusCase{
            Tool::Checkpoint,
            "1 world - lmb picks a tile - rmb takes"},
        StatusCase{
            Tool::Food,
            "1 world - lmb lays food to pick up - rmb takes it"},
        StatusCase{
            Tool::Water,
            "1 world - lmb lays water to pick up - rmb takes it"},
        StatusCase{
            Tool::Eraser,
            "1 world - lmb clears cubes - drag sweeps them away"}};

    class WorldViewTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        FakeEditSteps steps;
        FakeNotices notices;
        ViewHarness harness{logger, steps, notices};
        WorldView view;
    };

}

TEST_F(WorldViewTest, GetStatusText_NamesTheHandOfEveryTool)
{
    ASSERT_EQ(kStatusCases.size(), antwika::enums::kCount<Tool>);

    for (const auto &statusCase : kStatusCases)
    {
        harness.preferences.tool = statusCase.tool;

        EXPECT_EQ(
            view.getStatusText(harness.contextNow()),
            std::string(statusCase.text)
                + " - wheel zooms - g - c off - level 0");
    }
}

TEST_F(WorldViewTest, GetStatusText_ShowsTheCornerSeamsSwitchedOn)
{
    harness.preferences.tool = Tool::Brush;
    view.worldEdit().setCornerJoining(true);

    EXPECT_EQ(
        view.getStatusText(harness.contextNow()),
        "1 world - lmb adds - rmb takes - f5 plays"
        " - wheel zooms - g - c on - level 0");
}

TEST_F(WorldViewTest, GetStatusText_CarriesTheEditLevelAtTheEnd)
{
    harness.preferences.tool = Tool::Eraser;
    view.worldEdit().setEditLevel(-7);

    EXPECT_EQ(
        view.getStatusText(harness.contextNow()),
        "1 world - lmb clears cubes - drag sweeps them away"
        " - wheel zooms - g - c off - level -7");
}

namespace
{

    constexpr antwika::gfx::Size kWindowSize{.width = 960, .height = 540};

    constexpr antwika::gfx::Size kCanvasSize{.width = 480, .height = 270};

    void handsOutResources(NiceMock<MockRenderer> &innerRenderer)
    {
        ON_CALL(innerRenderer, createShader(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const ShaderSource &source)
                {
                    return std::unique_ptr<antwika::gfx::IShader>{
                        std::make_unique<NiceMock<MockShader>>()};
                });
        ON_CALL(innerRenderer, createMesh(::testing::_))
            .WillByDefault(
                []([[maybe_unused]] const antwika::gfx::MeshData &data)
                {
                    return std::unique_ptr<antwika::gfx::IMesh>{
                        std::make_unique<NiceMock<MockMesh>>()};
                });
        ON_CALL(innerRenderer, createRenderTarget(::testing::_))
            .WillByDefault(
                [](const antwika::gfx::RenderTargetSpec &spec)
                {
                    return std::unique_ptr<antwika::gfx::IRenderTarget>{
                        std::make_unique<FakeSizedTarget>(spec.size)};
                });
    }

    class WorldViewDrawTest : public ::testing::Test
    {
    protected:
        WorldViewDrawTest()
        {
            handsOutResources(innerRenderer);
            ON_CALL(innerRenderer, drawLine(
                ::testing::_, ::testing::_, ::testing::_))
                .WillByDefault(
                    [this](PointF, PointF, const Color color)
                    { lineColors.push_back(color); });
            ON_CALL(innerRenderer, beginClip(::testing::_))
                .WillByDefault(
                    [this](const antwika::gfx::RectF areaRect)
                    { clipRects.push_back(areaRect); });

            worldShader.open(
                viewportRenderer,
                ShaderSource{.vertex = "v", .fragment = "f"});
            scenePass.open(
                viewportRenderer,
                ShaderSource{.vertex = "v", .fragment = "f"});

            harness.cameraRig.viewHeight = 48.0F;
            view.setGrowTrouble({antwika::voxel::VoxelPosition{}});
        }

        [[nodiscard]] ViewContext contextNow() noexcept
        {
            auto shownContext = harness.contextNow();

            return ViewContext{
                .document = shownContext.document,
                .play = shownContext.play,
                .cameraRig = shownContext.cameraRig,
                .caption = shownContext.caption,
                .meters = shownContext.meters,
                .clockSource = shownContext.clockSource,
                .workbench = shownContext.workbench,
                .render =
                    antwika::editor::WorldRender{
                        .viewportRenderer = viewportRenderer,
                        .atlasSheets = shownContext.render.atlasSheets,
                        .worldMeshes = shownContext.render.worldMeshes,
                        .worldShader = worldShader,
                        .sprites = shownContext.render.sprites,
                        .scenePass = scenePass,
                        .lightPasses = shownContext.render.lightPasses,
                        .characterSkins = shownContext.render.characterSkins},
                .editSteps = shownContext.editSteps,
                .notices = shownContext.notices,
                .shownView = shownContext.shownView,
                .heldModifiers = shownContext.heldModifiers,
                .tick = shownContext.tick};
        }

        [[nodiscard]] std::size_t getTroubleLineCount() const noexcept
        {
            return static_cast<std::size_t>(
                std::count(
                    lineColors.begin(),
                    lineColors.end(),
                    antwika::editor::kForbiddenMarkerColor));
        }

        void standPlayer()
        {
            auto &world = harness.play.world;
            const auto player = world.create();

            antwika::ecs::OpenPhase phase(world);

            world.add<antwika::component::Position>(
                player, antwika::component::Position{});
            phase.close();
            harness.play.game->setPlayer(player);
        }

        [[nodiscard]] static antwika::ui::Frame getFrameWithWorldPanel(
            const antwika::gfx::Rect panelRect)
        {
            antwika::ui::Frame frame;

            frame.rects.widgetRects.push_back(
                antwika::ui::WidgetRect{
                    .widgetId = antwika::editor::kWorldPanelWidget,
                    .rect = panelRect});

            return frame;
        }

        NiceMock<MockLogger> logger;
        FakeEditSteps steps;
        FakeNotices notices;
        ViewHarness harness{logger, steps, notices};
        WorldView view;
        std::vector<antwika::gfx::RectF> clipRects;
        NiceMock<MockRenderer> innerRenderer;
        ViewportRenderer viewportRenderer{
            innerRenderer, kWindowSize, kCanvasSize};
        antwika::render::WorldShader worldShader;
        antwika::render::ScenePass scenePass;
        std::vector<Color> lineColors;
    };

}

TEST_F(WorldViewDrawTest, Draw_RulesTheGrowTroubleWhileEditing)
{
    harness.play.playing = false;

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_GT(getTroubleLineCount(), 0U);
}

TEST_F(WorldViewDrawTest, Draw_KeepsTheGrowTroubleOutOfPlayMode)
{
    standPlayer();
    harness.play.playing = true;

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(getTroubleLineCount(), 0U);
}

TEST_F(WorldViewDrawTest, Draw_CrossesACheckpointInWhiteInsteadOfACube)
{
    harness.play.playing = false;
    harness.document.map.markers
        .positionsOf(antwika::map::Marker::Checkpoint)
        .push_back(antwika::voxel::VoxelPosition{});

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(
        static_cast<std::size_t>(
            std::count(
                lineColors.begin(),
                lineColors.end(),
                antwika::editor::kWhiteColor)),
        3U);
}

TEST_F(WorldViewDrawTest, Draw_HangsTheCheckpointGizmoInPlaceOfTheCross)
{
    harness.play.playing = false;
    harness.document.map.markers
        .positionsOf(antwika::map::Marker::Checkpoint)
        .push_back(antwika::voxel::VoxelPosition{});
    antwika::editor::setIconPixel(
        harness.gizmos.sheetBitmap,
        antwika::enums::index(antwika::editor::GizmoKind::Checkpoint),
        {.column = 8, .row = 8},
        antwika::editor::kWhiteColor);

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(
        static_cast<std::size_t>(
            std::count(
                lineColors.begin(),
                lineColors.end(),
                antwika::editor::kWhiteColor)),
        0U);
}

TEST_F(WorldViewDrawTest, Draw_CrossesALampInWhiteWithNoGizmoDrawn)
{
    harness.play.playing = false;
    harness.document.map.lamps.push_back(antwika::light::Lamp{});

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(
        static_cast<std::size_t>(
            std::count(
                lineColors.begin(),
                lineColors.end(),
                antwika::editor::kWhiteColor)),
        3U);
}

TEST_F(WorldViewDrawTest, Draw_HangsTheLampGizmoInPlaceOfTheLampCross)
{
    harness.play.playing = false;
    harness.document.map.lamps.push_back(antwika::light::Lamp{});
    antwika::editor::setIconPixel(
        harness.gizmos.sheetBitmap,
        antwika::enums::index(antwika::editor::GizmoKind::Lamp),
        {.column = 8, .row = 8},
        antwika::editor::kWhiteColor);

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(
        static_cast<std::size_t>(
            std::count(
                lineColors.begin(),
                lineColors.end(),
                antwika::editor::kWhiteColor)),
        0U);
}

TEST_F(WorldViewDrawTest, Draw_HangsADrawnGizmoInPlaceOfTheCross)
{
    harness.play.playing = false;
    harness.document.map.spawnCubePosition =
        antwika::voxel::VoxelPosition{};
    antwika::editor::setIconPixel(
        harness.gizmos.sheetBitmap,
        antwika::enums::index(antwika::editor::GizmoKind::Spawn),
        {.column = 8, .row = 8},
        antwika::editor::kWhiteColor);

    view.draw(contextNow(), antwika::ui::Frame{});

    EXPECT_EQ(
        static_cast<std::size_t>(
            std::count(
                lineColors.begin(),
                lineColors.end(),
                antwika::editor::kWhiteColor)),
        0U);
}

TEST_F(WorldViewDrawTest, Draw_CutsTheWorldToTheWholeCanvasWithNoPanel)
{
    view.draw(contextNow(), antwika::ui::Frame{});

    ASSERT_FALSE(clipRects.empty());
    EXPECT_EQ(
        clipRects.front(),
        (antwika::gfx::RectF(
            {0.0F, 0.0F},
            {static_cast<float>(kWindowSize.width),
             static_cast<float>(kWindowSize.height)})));
}

TEST_F(WorldViewDrawTest, Draw_CutsTheWorldToThePanelTheFrameNames)
{
    view.draw(
        contextNow(),
        getFrameWithWorldPanel(
            antwika::gfx::Rect{
                .originPoint = {.x = 200, .y = 100},
                .size = {.width = 400, .height = 300}}));

    ASSERT_FALSE(clipRects.empty());
    EXPECT_EQ(
        clipRects.front(),
        (antwika::gfx::RectF({200.0F, 100.0F}, {400.0F, 300.0F})));
}
