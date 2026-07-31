#include "antwika/ui/Hover.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

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

    // Two boxes side by side, each drawn idle, each hoverable.
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
} // namespace

TEST(HoverTest, ApplyHover_ChangesNothingWhenNothingReportsAPosition)
{
    // The whole no-opt-in guarantee, and the one that matters most.
    // A caller that never passes a hover pointer draws what it built.
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
    // Lighting one up alone would leave the others as they were.
    // So whatever the recorded pointer last passed over stays lit.
    auto commands = DrawList{
        FillRect{.rect = boxAt(0), .color = kHovered},
        FillRect{.rect = boxAt(20), .color = kIdle}};

    applyHover(commands, twoTargets(), over(25, 5));

    EXPECT_EQ(kIdle, colorAt(commands, 0));
    EXPECT_EQ(kHovered, colorAt(commands, 1));
}

TEST(HoverTest, ApplyHover_TakesTheFrontmostOfTwoOverlappingTargets)
{
    // Paint order, so the later one is in front.
    // The hit-test reads that same order backwards to say the same.
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

TEST(HoverTest, ApplyHover_LeavesAHeldTargetLookingPressed)
{
    // A press is recorded input, resolved inside the tick path.
    // So its appearance is the simulation's answer, not a hint's.
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
    // The pointer is on the second box while the first is held.
    // The held one keeps its appearance and the other still answers.
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
    // The same rule the pointer hit-test uses, and the one function.
    // So a hover and a press agree about two touching widgets.
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
    // A picture and a list of targets are two values a caller pairs.
    // A mismatched pair is a thing to be handed, not one to police.
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
    // Absent rather than an origin, because an origin is a real place.
    EXPECT_EQ(HoverPointer{}, HoverPointer{});
    EXPECT_FALSE(HoverPointer{}.position.has_value());
    EXPECT_NE(HoverPointer{}, over(0, 0));
}

TEST(HoverTest, HoverTarget_ComparesEveryFieldItCarries)
{
    auto target = twoTargets().at(0);
    const auto same = target;

    EXPECT_EQ(same, target);

    target.held = true;

    EXPECT_NE(same, target);
}
