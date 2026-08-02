#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Frame.hpp"
#include "antwika/ui/Hover.hpp"
#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/HoverTargets.hpp"
#include "antwika/ui/Interactions.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::applyHover;
using antwika::ui::ButtonState;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::HoverPointer;
using antwika::ui::HoverTargets;
using antwika::ui::Interactions;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kIdle{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHovered{.red = 70, .green = 80, .blue = 90};
    constexpr Color kPressed{.red = 15, .green = 25, .blue = 35};

    constexpr WidgetId kLeft{1};
    constexpr WidgetId kRight{2};
    constexpr WidgetId kField{3};
    constexpr WidgetId kList{4};

    constexpr Size kCanvas{.width = 100, .height = 50};

    // Two two-glyph buttons in a row, no padding and no gap.
    // So the left one is x 0..11 and the right one x 12..23.
    constexpr Point kOnTheLeft{.x = 5, .y = 4};
    constexpr Point kOnTheRight{.x = 17, .y = 4};
    constexpr Point kOnNeither{.x = 80, .y = 40};

    Theme plainTheme()
    {
        return Theme{
            .buttonIdle = kIdle,
            .buttonHovered = kHovered,
            .buttonPressed = kPressed,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    // The frame the demo shape produces: two buttons side by side.
    [[nodiscard]] Frame twoButtons(Pointer pointer = {})
    {
        Context ui{kCanvas, plainTheme(), pointer};

        {
            const auto actions = ui.row();

            ui.button("ab", {.id = kLeft});
            ui.button("cd", {.id = kRight});
        }

        return ui.finish();
    }

    // Every answer Interactions carries, in one frame.
    // A focused field with typing, and an open list of named options.
    [[nodiscard]] Frame everything(Pointer pointer)
    {
        static constexpr std::array<std::string_view, 2> kOptions{
            "one", "two"};
        static constexpr std::string_view kHeld{"ab"};

        Context ui{
            kCanvas,
            plainTheme(),
            pointer,
            // A character, and no Backspace after it.
            // A frame putting the field back as it was reports nothing.
            Keyboard{.keys = {Key::Character}, .typed = "c"},
            kField};

        ui.textField({.id = kField, .text = kHeld, .focused = true});
        ui.dropdown({
            .id = kList,
            .optionIdBase = kLeft,
            .options = kOptions,
            .open = true});

        return ui.finish();
    }

    [[nodiscard]] Color fillOf(const DrawList &commands, std::size_t at)
    {
        return std::get<FillRect>(commands.at(at)).color;
    }

    [[nodiscard]] HoverPointer at(Point point)
    {
        return HoverPointer{.position = point};
    }

    // Every widget a hover pass could recolour, by name.
    [[nodiscard]] std::vector<WidgetId> idsOf(const HoverTargets &targets)
    {
        std::vector<WidgetId> ids;

        for (const auto &target : targets)
        {
            ids.push_back(target.id);
        }

        return ids;
    }
} // namespace

TEST(ContextHoverTest, Finish_ReportsOneTargetPerInteractiveNamedWidget)
{
    const auto frame = twoButtons();

    EXPECT_EQ(
        (std::vector<WidgetId>{kLeft, kRight}),
        idsOf(frame.hoverTargets));
}

TEST(ContextHoverTest, Finish_PlacesEveryTargetWhereTheLayoutPutIt)
{
    // Read off the arranging pass, as the widget rectangles are.
    // So a hover tests against the rectangle drawn from.
    const auto frame = twoButtons();

    for (const auto &target : frame.hoverTargets)
    {
        EXPECT_EQ(frame.rects.find(target.id), target.rect);
    }
}

TEST(ContextHoverTest, Finish_NamesTheCommandThatFillsEachWidget)
{
    const auto frame = twoButtons();

    for (const auto &target : frame.hoverTargets)
    {
        EXPECT_EQ(target.idle, fillOf(frame.commands, target.command));
    }
}

TEST(ContextHoverTest, Finish_ReportsNoTargetForAButtonTheCallerDressed)
{
    // A button told how to look was told, and that is the end of it.
    Context ui{kCanvas, plainTheme()};

    ui.button("ab", {.id = kLeft, .state = ButtonState::Idle});

    EXPECT_TRUE(ui.finish().hoverTargets.empty());
}

TEST(ContextHoverTest, Finish_ReportsNoTargetForAnUnnamedButton)
{
    // Nothing can hover what nothing can name.
    // Which is what resolve() already says inside the tick path.
    Context ui{kCanvas, plainTheme()};

    ui.button("ab");

    EXPECT_TRUE(ui.finish().hoverTargets.empty());
}

TEST(ContextHoverTest, Finish_ReportsNoTargetForAFrameWithNoWidgets)
{
    Context ui{kCanvas, plainTheme()};

    ui.label("just words");

    EXPECT_TRUE(ui.finish().hoverTargets.empty());
}

TEST(ContextHoverTest, ApplyHover_LightsAButtonTheEventStreamNeverSaw)
{
    // The point of the whole arrangement.
    // The UI's own pointer knows nothing, since the recording does.
    // And the button lights up all the same.
    auto frame = twoButtons();

    ASSERT_EQ(kNoWidget, frame.interactions.hovered);

    applyHover(frame.commands, frame.hoverTargets, at(kOnTheRight));

    EXPECT_EQ(kIdle, fillOf(frame.commands, frame.hoverTargets.at(0).command));
    EXPECT_EQ(
        kHovered, fillOf(frame.commands, frame.hoverTargets.at(1).command));
}

TEST(ContextHoverTest, ApplyHover_PutsOutWhatTheRecordedPointerLeftLit)
{
    // A gated stream carries a position only where a press needed one.
    // So the button under the last press would otherwise stay lit.
    auto frame = twoButtons(Pointer{.position = kOnTheLeft});

    ASSERT_EQ(kLeft, frame.interactions.hovered);

    applyHover(frame.commands, frame.hoverTargets, at(kOnTheRight));

    EXPECT_EQ(kIdle, fillOf(frame.commands, frame.hoverTargets.at(0).command));
    EXPECT_EQ(
        kHovered, fillOf(frame.commands, frame.hoverTargets.at(1).command));
}

TEST(ContextHoverTest, ApplyHover_LeavesAHeldButtonLookingPressed)
{
    auto frame = twoButtons(
        Pointer{.position = kOnTheLeft, .down = true});

    ASSERT_TRUE(frame.hoverTargets.at(0).held);
    ASSERT_FALSE(frame.hoverTargets.at(1).held);

    applyHover(frame.commands, frame.hoverTargets, at(kOnTheLeft));

    EXPECT_EQ(
        kPressed, fillOf(frame.commands, frame.hoverTargets.at(0).command));
}

TEST(ContextHoverTest, ApplyHover_DrawsTheSamePictureWithNoHoverPointer)
{
    // Byte for byte what finish() produced, for a caller not opting in.
    auto frame = twoButtons(Pointer{.position = kOnTheLeft});
    const auto before = frame.commands;

    applyHover(frame.commands, frame.hoverTargets, HoverPointer{});

    EXPECT_EQ(before, frame.commands);
}

TEST(ContextHoverTest, ApplyHover_LeavesEveryInteractionExactlyAsItWas)
{
    // The claim the whole design rests on.
    // Asserted over a frame with all four answers in play at once.
    // A press, a focused field an edit lands in, and a chosen option.
    const auto probe = everything(Pointer{});
    const auto option = probe.rects.find(kLeft);

    ASSERT_TRUE(option.has_value());

    auto frame = everything(Pointer{
        .position = Point{
            .x = option->origin.x + 1, .y = option->origin.y + 1},
        .pressed = true});

    const Interactions before = frame.interactions;

    // A frame where nothing happened would agree for the wrong reason.
    // So the answers being compared are first shown to be real.
    ASSERT_NE(kNoWidget, before.activated);
    ASSERT_NE(kNoWidget, before.focused);
    ASSERT_TRUE(before.edit.has_value());
    ASSERT_TRUE(before.chosen.has_value());

    for (const auto point : {kOnTheLeft, kOnTheRight, kOnNeither})
    {
        applyHover(frame.commands, frame.hoverTargets, at(point));

        EXPECT_EQ(before, frame.interactions);
    }
}

TEST(ContextHoverTest, Finish_PutsAnOpenListsOptionsInFrontOfTheBox)
{
    // Paint order across both layers, base first and overlay after.
    // So an open list's options win the hover against what they cover.
    const auto frame = everything(Pointer{});

    EXPECT_EQ(
        (std::vector<WidgetId>{kList, kLeft, kRight}),
        idsOf(frame.hoverTargets));
}
