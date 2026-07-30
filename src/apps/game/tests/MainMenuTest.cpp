#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/TextLayout.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/DrawList.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/Theme.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/MainMenu.hpp"
#include "antwika/game/MenuLabels.hpp"
#include "antwika/game/MenuState.hpp"

using antwika::game::kMenuEntries;
using antwika::game::kMenuLanguages;
using antwika::game::labelFor;
using antwika::game::labelsFor;
using antwika::game::MainMenu;
using antwika::game::MenuEntry;
using antwika::game::MenuLanguage;
using antwika::game::MenuState;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::gfx::textSize;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::ui::WidgetId;
namespace menuWidgets = antwika::game::menuWidgets;

namespace
{
    // Small enough that the whole picture fits in one expectation.
    // Below the 240 pixels scaleForCanvas() charges per glyph pixel.
    // So every metric in it is the theme's own.
    constexpr Size kCanvas{.width = 240, .height = 160};

    [[nodiscard]] std::vector<std::string> textsOf(const DrawList &commands)
    {
        std::vector<std::string> texts;

        for (const auto &command : commands)
        {
            if (const auto *text = std::get_if<DrawText>(&command))
            {
                texts.push_back(text->text);
            }
        }

        return texts;
    }

    // Where a widget sits is the layout's business.
    // So a test looks for a pixel that hits the one it means.
    [[nodiscard]] std::optional<Point> pointOn(
        WidgetId id, const MenuState &state)
    {
        const MainMenu menu;

        for (std::int32_t y = 0;
             y < static_cast<std::int32_t>(kCanvas.height);
             y += 2)
        {
            for (std::int32_t x = 0;
                 x < static_cast<std::int32_t>(kCanvas.width);
                 x += 2)
            {
                const Pointer pointer{.position = Point{.x = x, .y = y}};

                if (menu.describe(kCanvas, pointer, state)
                        .interactions.hovered
                    == id)
                {
                    return Point{.x = x, .y = y};
                }
            }
        }

        return std::nullopt;
    }
} // namespace

TEST(MainMenuTest, Describe_DrawsEveryEntryAndTheLanguageSelector)
{
    const MainMenu menu;
    const MenuState state{.gameBegun = true};
    const auto labels = labelsFor(MenuLanguage::English);

    const auto frame = menu.describe(kCanvas, Pointer{}, state);

    EXPECT_THAT(
        textsOf(frame.commands),
        ::testing::IsSupersetOf(
            {std::string{labels.title},
             std::string{labels.playGame},
             std::string{labels.loadReplay},
             std::string{labels.saveReplay},
             std::string{labels.resumeGame},
             std::string{labels.language},
             std::string{labels.english},
             std::string{labels.swedish}}));
}

// There is nothing to go back to until something has begun.
TEST(MainMenuTest, Describe_LeavesResumeOutBeforeAGameHasBegun)
{
    const MainMenu menu;
    const MenuState state;

    const auto frame = menu.describe(kCanvas, Pointer{}, state);

    EXPECT_THAT(
        textsOf(frame.commands),
        ::testing::Not(::testing::Contains(std::string{
            labelsFor(MenuLanguage::English).resumeGame})));
    EXPECT_EQ(
        std::nullopt, pointOn(menuWidgets::kResumeGame, state));
}

TEST(MainMenuTest, Describe_WritesEveryLabelInTheChosenLanguage)
{
    const MainMenu menu;
    const MenuState state{
        .gameBegun = true, .language = MenuLanguage::Swedish};
    const auto labels = labelsFor(MenuLanguage::Swedish);

    const auto frame = menu.describe(kCanvas, Pointer{}, state);

    EXPECT_THAT(
        textsOf(frame.commands),
        ::testing::IsSupersetOf(
            {std::string{labels.playGame},
             std::string{labels.loadReplay},
             std::string{labels.language}}));
}

TEST(MainMenuTest, Describe_HasAPixelForEveryEntryOnOffer)
{
    const MenuState state{.gameBegun = true};

    for (const auto entry : kMenuEntries)
    {
        EXPECT_TRUE(
            pointOn(antwika::game::widgetFor(entry), state).has_value())
            << "entry " << antwika::game::menuEntryIndex(entry);
    }
}

TEST(MainMenuTest, Describe_HasAPixelForEveryLanguage)
{
    const MenuState state;

    for (const auto language : kMenuLanguages)
    {
        EXPECT_TRUE(
            pointOn(
                antwika::game::widgetForLanguage(language), state)
                .has_value())
            << "language "
            << antwika::game::menuLanguageIndex(language);
    }
}

TEST(MainMenuTest, Describe_ReportsAPressOnTheEntryUnderThePointer)
{
    const MainMenu menu;
    const MenuState state;
    const auto at = pointOn(menuWidgets::kLoadReplay, state);
    ASSERT_TRUE(at.has_value());

    const auto frame = menu.describe(
        kCanvas,
        Pointer{.position = at, .down = true, .pressed = true},
        state);

    EXPECT_EQ(menuWidgets::kLoadReplay, frame.interactions.activated);
}

// A modal has to cover what it is over, or a click lands behind it.
TEST(MainMenuTest, Describe_CoversEveryCornerOfTheCanvas)
{
    const MainMenu menu;
    const MenuState state;

    const auto right = static_cast<std::int32_t>(kCanvas.width) - 1;
    const auto bottom = static_cast<std::int32_t>(kCanvas.height) - 1;

    for (const auto corner :
         {Point{.x = 0, .y = 0},
          Point{.x = right, .y = 0},
          Point{.x = 0, .y = bottom},
          Point{.x = right, .y = bottom}})
    {
        const auto frame = menu.describe(
            kCanvas,
            Pointer{.position = corner, .down = true, .pressed = true},
            state);

        EXPECT_TRUE(frame.interactions.pointerOverUi);
        EXPECT_EQ(kNoWidget, frame.interactions.activated);
    }
}

// Nothing here can clip, so the layout has to do the containing.
TEST(MainMenuTest, Describe_KeepsEveryWidgetInsideTheCanvas)
{
    const MainMenu menu;
    const MenuState state{.gameBegun = true};

    const auto right = static_cast<std::int32_t>(kCanvas.width);
    const auto bottom = static_cast<std::int32_t>(kCanvas.height);

    for (const auto &command :
         menu.describe(kCanvas, Pointer{}, state).commands)
    {
        if (const auto *fill = std::get_if<FillRect>(&command))
        {
            EXPECT_GE(fill->rect.origin.x, 0);
            EXPECT_GE(fill->rect.origin.y, 0);
            EXPECT_LE(
                fill->rect.origin.x
                    + static_cast<std::int32_t>(fill->rect.size.width),
                right);
            EXPECT_LE(
                fill->rect.origin.y
                    + static_cast<std::int32_t>(fill->rect.size.height),
                bottom);

            continue;
        }

        const auto &text = std::get<DrawText>(command);
        const auto extent = textSize(text.text, text.scale);

        EXPECT_GE(text.origin.x, 0);
        EXPECT_GE(text.origin.y, 0);
        EXPECT_LE(
            text.origin.x + static_cast<std::int32_t>(extent.width),
            right);
        EXPECT_LE(
            text.origin.y + static_cast<std::int32_t>(extent.height),
            bottom);
    }
}

// The selected language is shown as held down.
// Which one is in force is then readable without reading the words.
TEST(MainMenuTest, Describe_DrawsTheChosenLanguageAsPressed)
{
    const MainMenu menu;
    const auto theme = Theme{};

    const auto english =
        menu.describe(kCanvas, Pointer{}, MenuState{});
    const auto swedish = menu.describe(
        kCanvas, Pointer{}, MenuState{.language = MenuLanguage::Swedish});

    EXPECT_NE(english.commands, swedish.commands);
    EXPECT_THAT(
        english.commands,
        ::testing::Contains(::testing::VariantWith<FillRect>(
            ::testing::Field(&FillRect::color, theme.buttonPressed))));
}

// The whole picture, written out.
// A ui::Frame is a value so that this needs no renderer to assert.
// A layout changing under the menu is then a failure here.
TEST(MainMenuTest, Describe_DrawsExactlyThisPicture)
{
    const MainMenu menu;
    const Theme theme;

    const auto frame = menu.describe(
        kCanvas, Pointer{}, MenuState{.gameBegun = true});

    const DrawList expected{
        FillRect{
            .rect =
                {.origin = {.x = 0, .y = 0},
                 .size = {.width = 240, .height = 160}},
            .color = theme.panel},
        DrawText{
            .origin = {.x = 99, .y = 4},
            .text = "Antwika",
            .scale = 1,
            .color = theme.text},
        FillRect{
            .rect =
                {.origin = {.x = 87, .y = 16},
                 .size = {.width = 66, .height = 20}},
            .color = theme.buttonIdle},
        DrawText{
            .origin = {.x = 93, .y = 22},
            .text = "Play game",
            .scale = 1,
            .color = theme.buttonText},
        FillRect{
            .rect =
                {.origin = {.x = 81, .y = 40},
                 .size = {.width = 78, .height = 20}},
            .color = theme.buttonIdle},
        DrawText{
            .origin = {.x = 87, .y = 46},
            .text = "Load replay",
            .scale = 1,
            .color = theme.buttonText},
        FillRect{
            .rect =
                {.origin = {.x = 81, .y = 64},
                 .size = {.width = 78, .height = 20}},
            .color = theme.buttonIdle},
        DrawText{
            .origin = {.x = 87, .y = 70},
            .text = "Save replay",
            .scale = 1,
            .color = theme.buttonText},
        FillRect{
            .rect =
                {.origin = {.x = 81, .y = 88},
                 .size = {.width = 78, .height = 20}},
            .color = theme.buttonIdle},
        DrawText{
            .origin = {.x = 87, .y = 94},
            .text = "Resume game",
            .scale = 1,
            .color = theme.buttonText},
        DrawText{
            .origin = {.x = 96, .y = 112},
            .text = "Language",
            .scale = 1,
            .color = theme.text},
        FillRect{
            .rect =
                {.origin = {.x = 64, .y = 124},
                 .size = {.width = 54, .height = 20}},
            .color = theme.buttonPressed},
        DrawText{
            .origin = {.x = 70, .y = 130},
            .text = "English",
            .scale = 1,
            .color = theme.buttonText},
        FillRect{
            .rect =
                {.origin = {.x = 122, .y = 124},
                 .size = {.width = 54, .height = 20}},
            .color = theme.buttonIdle},
        DrawText{
            .origin = {.x = 128, .y = 130},
            .text = "Svenska",
            .scale = 1,
            .color = theme.buttonText}};

    EXPECT_EQ(expected, frame.commands);
}
