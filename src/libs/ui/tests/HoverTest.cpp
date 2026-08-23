#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/ui/Hover.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/HoverTarget.hpp"
#include "antwika/ui/HoverTargets.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::ui::applyHover;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::HoverPointer;
using antwika::ui::HoverTarget;
using antwika::ui::HoverTargets;
using antwika::widget::kNoWidget;
using antwika::widget::WidgetId;

namespace
{
    constexpr Color kIdleColor{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHoveredColor{.red = 70, .green = 80, .blue = 90};
    constexpr Color kHeldColor{.red = 15, .green = 25, .blue = 35};

    constexpr WidgetId kFirstWidget{1};
    constexpr WidgetId kSecondWidget{2};

    [[nodiscard]] Rect boxAt(std::int32_t x)
    {
        return Rect{
            .originPoint = {.x = x, .y = 0},
            .size = {.width = 10, .height = 10}};
    }

    [[nodiscard]] DrawList twoBoxes()
    {
        return DrawList{
            FillRect{.rect = boxAt(0), .color = kIdleColor},
            FillRect{.rect = boxAt(20), .color = kIdleColor}};
    }

    [[nodiscard]] HoverTargets twoTargets()
    {
        return HoverTargets{
            HoverTarget{
                .widgetId = kFirstWidget,
                .rect = boxAt(0),
                .command = 0,
                .idleColor = kIdleColor,
                .hoveredColor = kHoveredColor},
            HoverTarget{
                .widgetId = kSecondWidget,
                .rect = boxAt(20),
                .command = 1,
                .idleColor = kIdleColor,
                .hoveredColor = kHoveredColor}};
    }

    [[nodiscard]] Color colorAt(const DrawList &drawList,
        std::size_t commandIndex)
    {
        return std::get<FillRect>(drawList.at(commandIndex)).color;
    }

    [[nodiscard]] HoverPointer over(std::int32_t x, std::int32_t y)
    {
        return HoverPointer{.positionPoint = Point{.x = x, .y = y}};
    }
}

TEST(HoverTest, ApplyHover_ChangesNothingWhenNothingReportsAPosition)
{
    auto commands = twoBoxes();

    applyHover(commands, twoTargets(), HoverPointer{});

    EXPECT_EQ(twoBoxes(), commands);
}

TEST(HoverTest, ApplyHover_PaintsTheTargetUnderThePointerHovered)
{
    auto commands = twoBoxes();

    applyHover(commands, twoTargets(), over(5, 5));

    EXPECT_EQ(kHoveredColor, colorAt(commands, 0));
    EXPECT_EQ(kIdleColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_PutsEveryTargetItIsNotOverBackToIdle)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHoveredColor},
        FillRect{.rect = boxAt(20), .color = kIdleColor}};

    applyHover(commands, twoTargets(), over(25, 5));

    EXPECT_EQ(kIdleColor, colorAt(commands, 0));
    EXPECT_EQ(kHoveredColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_TakesTheFrontmostOfTwoOverlappingTargets)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kIdleColor},
        FillRect{.rect = boxAt(0), .color = kIdleColor}};

    const HoverTargets targets{
        HoverTarget{
            .widgetId = kFirstWidget,
            .rect = boxAt(0),
            .command = 0,
            .idleColor = kIdleColor,
            .hoveredColor = kHoveredColor},
        HoverTarget{
            .widgetId = kSecondWidget,
            .rect = boxAt(0),
            .command = 1,
            .idleColor = kIdleColor,
            .hoveredColor = kHoveredColor}};

    applyHover(commands, targets, over(5, 5));

    EXPECT_EQ(kIdleColor, colorAt(commands, 0));
    EXPECT_EQ(kHoveredColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_LightsEveryTargetSharingTheFrontmostsId)
{
    auto commands = twoBoxes();

    auto targets = twoTargets();
    targets.at(0).widgetId = kFirstWidget;
    targets.at(1).widgetId = kFirstWidget;

    applyHover(commands, targets, over(25, 5));

    EXPECT_EQ(kHoveredColor, colorAt(commands, 0));
    EXPECT_EQ(kHoveredColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_NeverPairsTwoUnnamedTargets)
{
    auto commands = twoBoxes();

    auto targets = twoTargets();
    targets.at(0).widgetId = kNoWidget;
    targets.at(1).widgetId = kNoWidget;

    applyHover(commands, targets, over(25, 5));

    EXPECT_EQ(kIdleColor, colorAt(commands, 0));
    EXPECT_EQ(kHoveredColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_LeavesAHeldTargetLookingPressed)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHeldColor},
        FillRect{.rect = boxAt(20), .color = kIdleColor}};

    auto targets = twoTargets();
    targets.at(0).held = true;

    applyHover(commands, targets, over(5, 5));

    EXPECT_EQ(kHeldColor, colorAt(commands, 0));
    EXPECT_EQ(kIdleColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_LetsAHeldTargetBeDraggedOffWithoutLightingUp)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHeldColor},
        FillRect{.rect = boxAt(20), .color = kIdleColor}};

    auto targets = twoTargets();
    targets.at(0).held = true;

    applyHover(commands, targets, over(25, 5));

    EXPECT_EQ(kHeldColor, colorAt(commands, 0));
    EXPECT_EQ(kHoveredColor, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_TreatsAWidgetsAreaAsHalfOpen)
{
    auto commands = twoBoxes();

    applyHover(commands, twoTargets(), over(9, 9));
    EXPECT_EQ(kHoveredColor, colorAt(commands, 0));

    applyHover(commands, twoTargets(), over(10, 9));
    EXPECT_EQ(kIdleColor, colorAt(commands, 0));
}

TEST(HoverTest, ApplyHover_ChangesNothingWithNoTargets)
{
    auto commands = twoBoxes();

    applyHover(commands, HoverTargets{}, over(5, 5));

    EXPECT_EQ(twoBoxes(), commands);
}

TEST(HoverTest, ApplyHover_SkipsATargetNamingACommandThatIsNotThere)
{
    auto commands = twoBoxes();

    HoverTargets targets{twoTargets().at(0)};
    targets.at(0).command = 7;

    applyHover(commands, targets, over(5, 5));

    EXPECT_EQ(twoBoxes(), commands);
}

TEST(HoverTest, ApplyHover_SkipsATargetNamingSomethingThatIsNotAFill)
{
    DrawList drawList{
        DrawText{
            .originPoint = {.x = 0, .y = 0},
            .text = std::string{"hi"},
            .scale = 1,
            .color = kIdleColor}};

    HoverTargets targets{twoTargets().at(0)};

    applyHover(drawList, targets, over(5, 5));

    EXPECT_EQ(kIdleColor, std::get<DrawText>(drawList.at(0)).color);
}

TEST(HoverTest, HoverPointer_ReportsNoPositionByDefault)
{
    const HoverPointer restingPointer{};
    const HoverPointer elsewherePointer{.positionPoint = std::nullopt};

    EXPECT_FALSE(restingPointer.positionPoint.has_value());
    EXPECT_EQ(restingPointer, elsewherePointer);
    EXPECT_NE(restingPointer, over(0, 0));
}

TEST(HoverTest, HoverTarget_ComparesEveryFieldItCarries)
{
    const auto original = twoTargets().at(0);

    EXPECT_EQ(original, twoTargets().at(0));

    using Change = void (*)(HoverTarget &);

    const std::array<Change, 6> changes{
        [](HoverTarget &target) { target.widgetId = kSecondWidget; },
        [](HoverTarget &target) { target.rect = boxAt(20); },
        [](HoverTarget &target) { target.command = 4; },
        [](HoverTarget &target) { target.idleColor = kHeldColor; },
        [](HoverTarget &target) { target.hoveredColor = kHeldColor; },
        [](HoverTarget &target) { target.held = true; }};

    for (const auto &change : changes)
    {
        auto changedTrack = original;
        change(changedTrack);

        EXPECT_NE(original, changedTrack);
    }
}
