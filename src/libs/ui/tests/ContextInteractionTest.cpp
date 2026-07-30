#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::ButtonState;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::FillRect;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kIdle{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHovered{.red = 70, .green = 80, .blue = 90};
    constexpr Color kPressed{.red = 15, .green = 25, .blue = 35};
    constexpr Color kPanel{.red = 10, .green = 20, .blue = 30};

    constexpr WidgetId kOk{1};

    constexpr Size kCanvas{.width = 100, .height = 50};

    // A two-glyph label at scale one, with no padding anywhere, puts
    // the button at (0, 0) and makes it twelve by eight.
    constexpr Point kOnTheButton{.x = 5, .y = 4};
    constexpr Point kOffTheButton{.x = 50, .y = 40};

    Theme plainTheme()
    {
        return Theme{
            .panel = kPanel,
            .buttonIdle = kIdle,
            .buttonHovered = kHovered,
            .buttonPressed = kPressed,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] Color fillColorOf(const DrawList &commands)
    {
        return std::get<FillRect>(commands.at(0)).color;
    }
} // namespace

TEST(ContextInteractionTest, Button_IsHoveredWhenThePointerIsOverIt)
{
    Context ui{kCanvas, plainTheme(), Pointer{.position = kOnTheButton}};

    ui.button("ab", {.id = kOk});

    const auto frame = ui.finish();

    EXPECT_EQ(kOk, frame.interactions.hovered);
    EXPECT_EQ(kHovered, fillColorOf(frame.commands));
}

TEST(ContextInteractionTest, Button_LooksPressedWhileItIsHeld)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{.position = kOnTheButton, .down = true}};

    ui.button("ab", {.id = kOk});

    EXPECT_EQ(kPressed, fillColorOf(ui.finish().commands));
}

TEST(ContextInteractionTest, Button_ActivatesOnThePress)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{.position = kOnTheButton, .down = true, .pressed = true}};

    ui.button("ab", {.id = kOk});

    EXPECT_EQ(kOk, ui.finish().interactions.activated);
}

TEST(ContextInteractionTest, Button_IgnoresAPressLandingElsewhere)
{
    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{.position = kOffTheButton, .down = true, .pressed = true}};

    ui.button("ab", {.id = kOk});

    const auto frame = ui.finish();

    EXPECT_EQ(kNoWidget, frame.interactions.activated);
    EXPECT_EQ(kIdle, fillColorOf(frame.commands));
}

TEST(ContextInteractionTest, Button_KeepsAnAppearanceItWasToldToHave)
{
    Context ui{kCanvas, plainTheme(), Pointer{.position = kOnTheButton}};

    ui.button("ab", {.id = kOk, .state = ButtonState::Idle});

    const auto frame = ui.finish();

    // Told how to look, but still something the pointer can be on.
    EXPECT_EQ(kIdle, fillColorOf(frame.commands));
    EXPECT_EQ(kOk, frame.interactions.hovered);
}

TEST(ContextInteractionTest, Button_IsNotHoveredWhenItWasNeverNamed)
{
    Context ui{kCanvas, plainTheme(), Pointer{.position = kOnTheButton}};

    ui.button("ab");

    const auto frame = ui.finish();

    EXPECT_EQ(kNoWidget, frame.interactions.hovered);
    EXPECT_EQ(kIdle, fillColorOf(frame.commands));
}

TEST(ContextInteractionTest, Finish_ReportsThePointerOverAPanel)
{
    Context ui{kCanvas, plainTheme(), Pointer{.position = kOffTheButton}};

    {
        const auto screen = ui.panel({.height = antwika::ui::kGrow});

        ui.label("ab");
    }

    EXPECT_TRUE(ui.finish().interactions.pointerOverUi);
}

TEST(ContextInteractionTest, Finish_ReportsNothingWithoutAPointer)
{
    Context ui{kCanvas, plainTheme()};

    ui.button("ab", {.id = kOk});

    const auto frame = ui.finish();

    EXPECT_EQ(kNoWidget, frame.interactions.hovered);
    EXPECT_FALSE(frame.interactions.pointerOverUi);
    EXPECT_EQ(kIdle, fillColorOf(frame.commands));
}
