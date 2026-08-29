#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string_view>

#include <antwika/gfx/NullBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/NullInputBackend.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/widget/WidgetId.hpp>

#include "antwika/editor/Editor.hpp"
#include "antwika/editor/fakes/EditorProbe.hpp"
#include "antwika/editor/ui/WidgetIds.hpp"

namespace
{

    using ::testing::NiceMock;
    using antwika::log::mocks::MockLogger;

    constexpr std::string_view kMissingMapPath = "no-such-map.json";

    class ComponentScrollTest : public ::testing::Test
    {
    protected:
        NiceMock<MockLogger> logger;
        antwika::gfx::NullBackend backend{logger};
        antwika::input::NullInputBackend inputs{logger};
        antwika::editor::Editor editor{
            logger, backend, inputs, std::string(kMissingMapPath)};
        antwika::editor::fakes::EditorProbe probe{editor};
    };

}

TEST_F(ComponentScrollTest, CarryComponentScroll_MovesTheInspectorWhereItIsSent)
{
    antwika::ui::Interactions interactions{};

    interactions.scrollChange = antwika::ui::ScrollChange{
        .areaWidget = antwika::editor::kComponentScrollWidget, .line = 12};

    probe.carryComponentScroll(interactions);

    EXPECT_EQ(probe.characterTool().getInspectorScroll(), 12U);
}

TEST_F(ComponentScrollTest, CarryComponentScroll_LeavesTheInspectorForOtherPanes)
{
    antwika::ui::Interactions interactions{};

    interactions.scrollChange = antwika::ui::ScrollChange{
        .areaWidget = antwika::editor::kStatusBarWidget, .line = 12};

    probe.carryComponentScroll(interactions);

    EXPECT_EQ(probe.characterTool().getInspectorScroll(), 0U);
}

TEST_F(ComponentScrollTest, OnScrolled_GathersWheelStepsUntilTheFrameTakesThem)
{
    probe.onScrolled(antwika::input::PointerScrolled{.vertical = -1});
    probe.onScrolled(antwika::input::PointerScrolled{.vertical = -1});

    EXPECT_EQ(probe.pointer.wheelSteps, 2);

    probe.pumpFrame();

    EXPECT_EQ(probe.pointer.wheelSteps, 0);
}

TEST_F(ComponentScrollTest, InspectorHover_KeepsTheWheelFromTheWorld)
{
    probe.pointer.hoveredWidget = antwika::editor::getComponentHeadWidget(0);

    EXPECT_TRUE(probe.isInspectorHovered());

    probe.pointer.hoveredWidget = antwika::editor::kComponentScrollWidget;

    EXPECT_TRUE(probe.isInspectorHovered());

    probe.pointer.hoveredWidget = antwika::widget::kNoWidget;

    EXPECT_FALSE(probe.isInspectorHovered());

    probe.pointer.hoveredWidget = antwika::editor::kStatusBarWidget;

    EXPECT_FALSE(probe.isInspectorHovered());
}
