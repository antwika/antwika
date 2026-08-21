#include <gtest/gtest.h>

#include <cstdint>
#include <variant>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Glyphs.hpp>
#include <antwika/gfx/mocks/MockTexture.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/ButtonState.hpp"
#include "antwika/ui/Context.hpp"
#include "antwika/ui/Icon.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/Theme.hpp"

using antwika::gfx::Color;
using antwika::gfx::Size;
using antwika::ui::ButtonState;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::fixedSize;
using antwika::ui::kGrowSizing;
using antwika::ui::Theme;

namespace
{
    constexpr Color kPanelColor{.red = 10, .green = 20, .blue = 30};
    constexpr Color kAccentColor{.red = 90, .green = 40, .blue = 40};
    constexpr Color kInkColor{.red = 200, .green = 210, .blue = 220};
    constexpr Color kMutedColor{.red = 100, .green = 110, .blue = 105};
    constexpr Color kIdleColor{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHoveredColor{.red = 70, .green = 80, .blue = 90};
    constexpr Color kPressedColor{.red = 15, .green = 25, .blue = 35};
    constexpr Color kButtonInkColor{.red = 250, .green = 250, .blue = 250};

    Theme plainTheme(std::uint32_t padding = 0)
    {
        return Theme{
            .panelColor = kPanelColor,
            .textColor = kInkColor,
            .mutedColor = kMutedColor,
            .buttonIdleColor = kIdleColor,
            .buttonHoveredColor = kHoveredColor,
            .buttonPressedColor = kPressedColor,
            .buttonTextColor = kButtonInkColor,
            .textScale = 1,
            .padding = padding,
            .gap = 0,
            .buttonPadding = 0};
    }

    constexpr Size kCanvasSize{.width = 100, .height = 50};

    constexpr std::uint32_t kGlyph = antwika::gfx::kGlyphAdvance;

    [[nodiscard]] Color fillColorOf(const DrawList &drawList)
    {
        return std::get<FillRect>(drawList.at(0)).color;
    }
}

TEST(ContextWidgetsTest, Label_UsesTheThemesTextColor)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.label("ab");

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(kInkColor, std::get<DrawText>(commands.at(0)).color);
}

TEST(ContextWidgetsTest, Label_TakesAColorOfItsOwn)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.label("ab", uiContext.theme().mutedColor);

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(kMutedColor, std::get<DrawText>(commands.at(0)).color);
}

TEST(ContextWidgetsTest, Button_DrawsAFilledBoxAroundItsLabel)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab");

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(
        (FillRect{
            .rect =
                {.originPoint = {.x = 0, .y = 0},
                 .size = {.width = 12, .height = 8}},
            .color = kIdleColor}),
        std::get<FillRect>(commands.at(0)));
    EXPECT_EQ(
        (DrawText{
            .originPoint = {.x = 0, .y = 0},
            .text = "ab",
            .scale = 1,
            .color = kButtonInkColor}),
        std::get<DrawText>(commands.at(1)));
}

TEST(ContextWidgetsTest, Button_BreaksItsNameOverLinesToFitAWidth)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button(
        "ab cd ef",
        {.widthSizing = fixedSize(6 * kGlyph), .wrapWidth = 6 * kGlyph});

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(3U, commands.size());
    EXPECT_EQ("ab cd", std::get<DrawText>(commands.at(1)).text);
    EXPECT_EQ("ef", std::get<DrawText>(commands.at(2)).text);
}

TEST(ContextWidgetsTest, Button_GrowsTallerForEveryLineItBreaksInto)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab");

    const auto one =
        std::get<FillRect>(uiContext.build().drawList.at(0)).rect.size.height;

    Context wrappedContext{kCanvasSize, plainTheme()};

    wrappedContext.button(
        "ab cd ef",
        {.widthSizing = fixedSize(6 * kGlyph), .wrapWidth = 6 * kGlyph});

    const auto two =
        std::get<FillRect>(wrappedContext.build().drawList.at(0))
            .rect.size.height;

    EXPECT_EQ(two, one * 2);
}

TEST(ContextWidgetsTest, Button_LeavesItsNameWholeWhenNoWidthIsGiven)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab cd ef");

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ("ab cd ef", std::get<DrawText>(commands.at(1)).text);
}

TEST(ContextWidgetsTest, Button_UsesTheHoveredFillWhenToldTo)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab", {.state = ButtonState::Hovered});

    EXPECT_EQ(kHoveredColor, fillColorOf(uiContext.build().drawList));
}

TEST(ContextWidgetsTest, Button_UsesThePressedFillWhenToldTo)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab", {.state = ButtonState::Pressed});

    EXPECT_EQ(kPressedColor, fillColorOf(uiContext.build().drawList));
}

TEST(ContextWidgetsTest, Button_CentresItsLabelWhenAskedToGrow)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.button("ab", {.widthSizing = kGrowSizing});

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(
        (FillRect{
            .rect =
                {.originPoint = {.x = 0, .y = 0},
                 .size = {.width = 100, .height = 8}},
            .color = kIdleColor}),
        std::get<FillRect>(commands.at(0)));

    EXPECT_EQ(
        44, std::get<DrawText>(commands.at(1)).originPoint.x);
}

TEST(ContextWidgetsTest, Spacer_PushesWhatFollowsToTheEndOfARow)
{
    Context uiContext{Size{.width = 100, .height = 10}, plainTheme()};

    {
        const auto row = uiContext.row({.heightSizing = fixedSize(10)});

        uiContext.spacer(kGrowSizing);
        uiContext.label("ab");
    }

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(88, std::get<DrawText>(commands.at(0)).originPoint.x);
}

TEST(ContextWidgetsTest, Spacer_PushesWhatFollowsToTheBottomOfAColumn)
{
    Context uiContext{Size{.width = 100, .height = 100}, plainTheme()};

    {
        const auto column = uiContext.column({.heightSizing = kGrowSizing});

        uiContext.spacer(kGrowSizing);
        uiContext.label("ab");
    }

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(1U, commands.size());
    EXPECT_EQ(92, std::get<DrawText>(commands.at(0)).originPoint.y);
}

TEST(ContextWidgetsTest, Panel_FillsInTheThemesBackgroundAndPadding)
{
    Context uiContext{kCanvasSize, plainTheme(4)};

    {
        const auto panel = uiContext.panel({.heightSizing = fixedSize(20)});

        uiContext.label("ab");
    }

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(kPanelColor, fillColorOf(commands));
    EXPECT_EQ(
        (antwika::gfx::Point{.x = 4, .y = 4}),
        std::get<DrawText>(commands.at(1)).originPoint);
}

TEST(ContextWidgetsTest, Panel_KeepsABackgroundAndPaddingAskedFor)
{
    Context uiContext{kCanvasSize, plainTheme(4)};

    {
        const auto panel = uiContext.panel({
            .heightSizing = fixedSize(20),
            .backgroundColor = kAccentColor,
            .padding = 0});

        uiContext.label("ab");
    }

    const auto commands = uiContext.build().drawList;

    ASSERT_EQ(2U, commands.size());
    EXPECT_EQ(kAccentColor, fillColorOf(commands));
    EXPECT_EQ(
        (antwika::gfx::Point{.x = 0, .y = 0}),
        std::get<DrawText>(commands.at(1)).originPoint);
}

TEST(ContextWidgetsTest, Image_TakesTheSizeOfWhatItShows)
{
    const antwika::gfx::mocks::MockTexture sheetTexture;
    const antwika::ui::Icon shownIcon{
        .sheetTexture = &sheetTexture,
        .sourceRect =
            antwika::gfx::Rect{
                .originPoint = {.x = 32, .y = 0},
                .size = {.width = 16, .height = 16}}};

    antwika::ui::Context uiContext{
        Size{.width = 80, .height = 40}, antwika::ui::Theme{}};

    uiContext.image(shownIcon, Color{.red = 200, .green = 200, .blue = 200});

    const auto frame = uiContext.build();

    auto drawnCount = 0;

    for (const auto &command : frame.drawList)
    {
        const auto *picture =
            std::get_if<antwika::ui::DrawTexture>(&command);

        if (picture == nullptr)
        {
            continue;
        }

        ++drawnCount;

        EXPECT_EQ(picture->texture, &sheetTexture);
        EXPECT_EQ(picture->sourceRect, shownIcon.sourceRect);
        EXPECT_EQ(picture->destinationRect.size, shownIcon.sourceRect.size);
    }

    EXPECT_EQ(drawnCount, 1);
}

TEST(ContextWidgetsTest, IconButton_PutsThePictureOnAButtonFace)
{
    const antwika::gfx::mocks::MockTexture sheetTexture;
    const antwika::ui::Icon shownIcon{
        .sheetTexture = &sheetTexture,
        .sourceRect =
            antwika::gfx::Rect{
                .originPoint = {},
                .size = {.width = 16, .height = 16}}};

    antwika::ui::Context uiContext{
        Size{.width = 80, .height = 40}, antwika::ui::Theme{}};

    uiContext.iconButton(shownIcon, antwika::ui::ButtonSpec{});

    const auto frame = uiContext.build();

    auto fills = 0;
    auto pictures = 0;

    for (const auto &command : frame.drawList)
    {
        if (std::holds_alternative<antwika::ui::FillRect>(command))
        {
            ++fills;
        }

        if (std::holds_alternative<antwika::ui::DrawTexture>(command))
        {
            ++pictures;
        }
    }

    EXPECT_GT(fills, 0);
    EXPECT_EQ(pictures, 1);
}

TEST(ContextWidgetsTest, IconButton_IsLargerThanThePictureItHolds)
{
    const antwika::gfx::mocks::MockTexture sheetTexture;
    const antwika::ui::Icon shownIcon{
        .sheetTexture = &sheetTexture,
        .sourceRect =
            antwika::gfx::Rect{
                .originPoint = {},
                .size = {.width = 16, .height = 16}}};

    antwika::ui::Context uiContext{
        Size{.width = 80, .height = 40}, antwika::ui::Theme{}};

    uiContext.iconButton(shownIcon, antwika::ui::ButtonSpec{});

    const auto frame = uiContext.build();

    for (const auto &command : frame.drawList)
    {
        const auto *fill =
            std::get_if<antwika::ui::FillRect>(&command);

        if (fill == nullptr)
        {
            continue;
        }

        EXPECT_GE(fill->rect.size.width, shownIcon.sourceRect.size.width);
        EXPECT_GE(fill->rect.size.height, shownIcon.sourceRect.size.height);
    }
}
