#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::scaledTheme;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kRing{.red = 200, .green = 210, .blue = 20};

    constexpr WidgetId kOne{1};
    constexpr WidgetId kTwo{2};

    constexpr Size kCanvas{.width = 100, .height = 50};

    constexpr Rect kFirstButton{
        .origin = {.x = 0, .y = 0}, .size = {.width = 12, .height = 8}};

    constexpr Point kOnTheButton{.x = 5, .y = 4};

    Theme plainTheme(std::uint32_t thickness)
    {
        return Theme{
            .focusRing = kRing,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0,
            .focusRingThickness = thickness};
    }

    DrawList ringAround(Rect box, std::uint32_t thickness)
    {
        const auto tall = std::min(thickness, box.size.height);
        const auto wide = std::min(thickness, box.size.width);

        const auto bottom = box.origin.y
                            + static_cast<std::int32_t>(
                                box.size.height - tall);
        const auto right = box.origin.x
                           + static_cast<std::int32_t>(
                               box.size.width - wide);

        return DrawList{
            FillRect{
                .rect =
                    {.origin = box.origin,
                     .size = {.width = box.size.width, .height = tall}},
                .color = kRing},
            FillRect{
                .rect =
                    {.origin = {.x = box.origin.x, .y = bottom},
                     .size = {.width = box.size.width, .height = tall}},
                .color = kRing},
            FillRect{
                .rect =
                    {.origin = box.origin,
                     .size = {.width = wide, .height = box.size.height}},
                .color = kRing},
            FillRect{
                .rect =
                    {.origin = {.x = right, .y = box.origin.y},
                     .size = {.width = wide, .height = box.size.height}},
                .color = kRing}};
    }

    DrawList tailOf(const DrawList &commands, std::size_t count)
    {
        return DrawList{commands.end() - count, commands.end()};
    }
}

TEST(ContextKeyboardTest, Context_DrawsTheSameThingWithoutAKeyboard)
{
    Context bare{kCanvas, plainTheme(2)};
    bare.button("ab", {.id = kOne});

    Context keyed{kCanvas, plainTheme(2), Pointer{}, Keyboard{}};
    keyed.button("ab", {.id = kOne});

    const auto expected = bare.finish();
    const auto actual = keyed.finish();

    EXPECT_FALSE(expected.commands.empty());
    EXPECT_EQ(expected.commands, actual.commands);
    EXPECT_EQ(expected.interactions, actual.interactions);
    EXPECT_EQ(kNoWidget, actual.interactions.focused);
}

TEST(ContextKeyboardTest, Context_TabFocusesTheFirstButton)
{
    Context ui{
        kCanvas,
        plainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    ui.button("ab", {.id = kOne});
    ui.button("cd", {.id = kTwo});

    EXPECT_EQ(kOne, ui.finish().interactions.focused);
}

TEST(ContextKeyboardTest, Context_TabCarriesOnFromTheFocusItWasGiven)
{
    Context ui{
        kCanvas,
        plainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}},
        kOne};

    ui.button("ab", {.id = kOne});
    ui.button("cd", {.id = kTwo});

    EXPECT_EQ(kTwo, ui.finish().interactions.focused);
}

TEST(ContextKeyboardTest, Context_SkipsAButtonNothingCanName)
{
    Context ui{
        kCanvas,
        plainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext}}};

    ui.button("ab");
    ui.button("cd", {.id = kTwo});

    EXPECT_EQ(kTwo, ui.finish().interactions.focused);
}

TEST(ContextKeyboardTest, Context_ActivatesTheFocusedButtonOnEnter)
{
    Context ui{
        kCanvas,
        plainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kTwo};

    ui.button("ab", {.id = kOne});
    ui.button("cd", {.id = kTwo});

    EXPECT_EQ(kTwo, ui.finish().interactions.activated);
}

TEST(ContextKeyboardTest, Context_HandsFocusOnToTheNextFrame)
{
    Context first{
        kCanvas,
        plainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext, Key::FocusNext}}};

    first.button("ab", {.id = kOne});
    first.button("cd", {.id = kTwo});

    const auto focus = first.finish().interactions.focused;

    Context second{
        kCanvas,
        plainTheme(2),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        focus};

    second.button("ab", {.id = kOne});
    second.button("cd", {.id = kTwo});

    EXPECT_EQ(kTwo, focus);
    EXPECT_EQ(kTwo, second.finish().interactions.activated);
}

TEST(ContextKeyboardTest, Context_DrawsFourBarsRoundTheFocusedButton)
{
    Context ui{kCanvas, plainTheme(3), Pointer{}, Keyboard{}, kOne};

    ui.button("ab", {.id = kOne});
    ui.button("cd", {.id = kTwo});

    const auto commands = ui.finish().commands;

    EXPECT_EQ(ringAround(kFirstButton, 3), tailOf(commands, 4));
}

TEST(ContextKeyboardTest, Context_ClampsTheRingToASmallerWidget)
{
    Context ui{kCanvas, plainTheme(40), Pointer{}, Keyboard{}, kOne};

    ui.button("ab", {.id = kOne, .width = fixedSize(4)});

    const auto commands = ui.finish().commands;
    const auto box = std::get<FillRect>(commands.at(0)).rect;

    EXPECT_EQ(4U, box.size.width);
    EXPECT_EQ(ringAround(box, 40), tailOf(commands, 4));
}

TEST(ContextKeyboardTest, Context_DrawsNoRingWhenTheThemeHasNoThickness)
{
    Context bare{kCanvas, plainTheme(0)};
    bare.button("ab", {.id = kOne});

    Context focused{kCanvas, plainTheme(0), Pointer{}, Keyboard{}, kOne};
    focused.button("ab", {.id = kOne});

    const auto expected = focused.finish();

    EXPECT_FALSE(expected.commands.empty());
    EXPECT_EQ(bare.finish().commands, expected.commands);
    EXPECT_EQ(kOne, expected.interactions.focused);
}

TEST(ContextKeyboardTest, Context_MovesTheRingToAClickedButton)
{
    Context ui{
        kCanvas,
        plainTheme(3),
        Pointer{.position = kOnTheButton, .pressed = true},
        Keyboard{},
        kTwo};

    ui.button("ab", {.id = kOne});
    ui.button("cd", {.id = kTwo});

    const auto frame = ui.finish();

    EXPECT_EQ(kOne, frame.interactions.focused);
    EXPECT_EQ(ringAround(kFirstButton, 3), tailOf(frame.commands, 4));
}

TEST(ContextKeyboardTest, Theme_DefaultsToAYellowRingTwoPixelsThick)
{
    constexpr Theme theme{};

    EXPECT_EQ(2U, theme.focusRingThickness);
    EXPECT_EQ(
        (Color{.red = 244, .green = 208, .blue = 63}), theme.focusRing);
}

TEST(ContextKeyboardTest, ScaledTheme_MultipliesTheRingThickness)
{
    constexpr Theme base{};

    const auto theme = scaledTheme(base, 3);

    EXPECT_EQ(6U, theme.focusRingThickness);
    EXPECT_EQ(base.focusRing, theme.focusRing);
}
