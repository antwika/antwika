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
using antwika::ui::kNoWidget;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kIdle{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHovered{.red = 70, .green = 80, .blue = 90};
    constexpr Color kHeld{.red = 15, .green = 25, .blue = 35};

    constexpr WidgetId kFirst{1};
    constexpr WidgetId kSecond{2};

    [[nodiscard]] Rect boxAt(std::int32_t x)
    {
        return Rect{
            .origin = {.x = x, .y = 0},
            .size = {.width = 10, .height = 10}};
    }

    [[nodiscard]] DrawList twoBoxes()
    {
        return DrawList{
            FillRect{.rect = boxAt(0), .color = kIdle},
            FillRect{.rect = boxAt(20), .color = kIdle}};
    }

    [[nodiscard]] HoverTargets twoTargets()
    {
        return HoverTargets{
            HoverTarget{
                .id = kFirst,
                .rect = boxAt(0),
                .command = 0,
                .idle = kIdle,
                .hovered = kHovered},
            HoverTarget{
                .id = kSecond,
                .rect = boxAt(20),
                .command = 1,
                .idle = kIdle,
                .hovered = kHovered}};
    }

    [[nodiscard]] Color colorAt(const DrawList &commands, std::size_t at)
    {
        return std::get<FillRect>(commands.at(at)).color;
    }

    [[nodiscard]] HoverPointer over(std::int32_t x, std::int32_t y)
    {
        return HoverPointer{.position = Point{.x = x, .y = y}};
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

    EXPECT_EQ(kHovered, colorAt(commands, 0));
    EXPECT_EQ(kIdle, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_PutsEveryTargetItIsNotOverBackToIdle)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHovered},
        FillRect{.rect = boxAt(20), .color = kIdle}};

    applyHover(commands, twoTargets(), over(25, 5));

    EXPECT_EQ(kIdle, colorAt(commands, 0));
    EXPECT_EQ(kHovered, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_TakesTheFrontmostOfTwoOverlappingTargets)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kIdle},
        FillRect{.rect = boxAt(0), .color = kIdle}};

    const HoverTargets targets{
        HoverTarget{
            .id = kFirst,
            .rect = boxAt(0),
            .command = 0,
            .idle = kIdle,
            .hovered = kHovered},
        HoverTarget{
            .id = kSecond,
            .rect = boxAt(0),
            .command = 1,
            .idle = kIdle,
            .hovered = kHovered}};

    applyHover(commands, targets, over(5, 5));

    EXPECT_EQ(kIdle, colorAt(commands, 0));
    EXPECT_EQ(kHovered, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_LightsEveryTargetSharingTheFrontmostsId)
{
    auto commands = twoBoxes();

    auto targets = twoTargets();
    targets.at(0).id = kFirst;
    targets.at(1).id = kFirst;

    applyHover(commands, targets, over(25, 5));

    EXPECT_EQ(kHovered, colorAt(commands, 0));
    EXPECT_EQ(kHovered, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_NeverPairsTwoUnnamedTargets)
{
    auto commands = twoBoxes();

    auto targets = twoTargets();
    targets.at(0).id = kNoWidget;
    targets.at(1).id = kNoWidget;

    applyHover(commands, targets, over(25, 5));

    EXPECT_EQ(kIdle, colorAt(commands, 0));
    EXPECT_EQ(kHovered, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_LeavesAHeldTargetLookingPressed)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHeld},
        FillRect{.rect = boxAt(20), .color = kIdle}};

    auto targets = twoTargets();
    targets.at(0).held = true;

    applyHover(commands, targets, over(5, 5));

    EXPECT_EQ(kHeld, colorAt(commands, 0));
    EXPECT_EQ(kIdle, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_LetsAHeldTargetBeDraggedOffWithoutLightingUp)
{
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHeld},
        FillRect{.rect = boxAt(20), .color = kIdle}};

    auto targets = twoTargets();
    targets.at(0).held = true;

    applyHover(commands, targets, over(25, 5));

    EXPECT_EQ(kHeld, colorAt(commands, 0));
    EXPECT_EQ(kHovered, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_TreatsAWidgetsAreaAsHalfOpen)
{
    auto commands = twoBoxes();

    applyHover(commands, twoTargets(), over(9, 9));
    EXPECT_EQ(kHovered, colorAt(commands, 0));

    applyHover(commands, twoTargets(), over(10, 9));
    EXPECT_EQ(kIdle, colorAt(commands, 0));
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
    DrawList commands{
        DrawText{
            .origin = {.x = 0, .y = 0},
            .text = std::string{"hi"},
            .scale = 1,
            .color = kIdle}};

    HoverTargets targets{twoTargets().at(0)};

    applyHover(commands, targets, over(5, 5));

    EXPECT_EQ(kIdle, std::get<DrawText>(commands.at(0)).color);
}

TEST(HoverTest, HoverPointer_ReportsNoPositionByDefault)
{
    const HoverPointer resting{};
    const HoverPointer elsewhere{.position = std::nullopt};

    EXPECT_FALSE(resting.position.has_value());
    EXPECT_EQ(resting, elsewhere);
    EXPECT_NE(resting, over(0, 0));
}

TEST(HoverTest, HoverTarget_ComparesEveryFieldItCarries)
{
    const auto original = twoTargets().at(0);

    EXPECT_EQ(original, twoTargets().at(0));

    using Change = void (*)(HoverTarget &);

    const std::array<Change, 6> changes{
        [](HoverTarget &target) { target.id = kSecond; },
        [](HoverTarget &target) { target.rect = boxAt(20); },
        [](HoverTarget &target) { target.command = 4; },
        [](HoverTarget &target) { target.idle = kHeld; },
        [](HoverTarget &target) { target.hovered = kHeld; },
        [](HoverTarget &target) { target.held = true; }};

    for (const auto &change : changes)
    {
        auto changed = original;
        change(changed);

        EXPECT_NE(original, changed);
    }
}
