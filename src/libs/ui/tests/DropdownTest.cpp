#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/ui/Context.hpp"
#include "antwika/ui/DrawCommand.hpp"
#include "antwika/ui/DrawList.hpp"
#include "antwika/ui/DropdownSpec.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Theme.hpp"
#include "antwika/ui/WidgetId.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::Context;
using antwika::ui::DrawList;
using antwika::ui::DrawText;
using antwika::ui::DropdownSpec;
using antwika::ui::FillRect;
using antwika::ui::kNoOption;
using antwika::ui::kNoWidget;
using antwika::ui::OptionChoice;
using antwika::ui::Pointer;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kPanel{.red = 10, .green = 20, .blue = 30};
    constexpr Color kIdle{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHovered{.red = 70, .green = 80, .blue = 90};
    constexpr Color kButtonInk{.red = 250, .green = 250, .blue = 250};

    constexpr WidgetId kPicker{5};
    constexpr WidgetId kFirstOption{100};
    constexpr WidgetId kBelow{9};

    constexpr Size kCanvas{.width = 200, .height = 100};

    constexpr std::array<std::string_view, 2> kSaves{"one", "two"};

    Theme plainTheme()
    {
        return Theme{
            .panel = kPanel,
            .buttonIdle = kIdle,
            .buttonHovered = kHovered,
            .buttonText = kButtonInk,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] DropdownSpec pickerSpec()
    {
        return DropdownSpec{
            .id = kPicker,
            .optionIdBase = kFirstOption,
            .options = kSaves,
            .selected = 0};
    }

    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &commands)
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
} // namespace

TEST(DropdownTest, ClosedItShowsOnlyWhatIsSelected)
{
    Context ui{kCanvas, plainTheme()};

    ui.dropdown(pickerSpec());

    const auto texts = textsOf(ui.finish().commands);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("v", texts.at(1));
}

TEST(DropdownTest, ClosedWithNothingSelectedItShowsThePlaceholder)
{
    auto spec = pickerSpec();
    spec.selected = kNoOption;
    spec.placeholder = "pick";

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);

    EXPECT_EQ("pick", textsOf(ui.finish().commands).at(0));
}

TEST(DropdownTest, OpenItDrawsEveryOptionUnderTheBox)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);

    const auto commands = ui.finish().commands;
    const auto texts = textsOf(commands);

    ASSERT_EQ(4U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("v", texts.at(1));
    EXPECT_EQ("one", texts.at(2));
    EXPECT_EQ("two", texts.at(3));

    // The list hangs below the box rather than over it.
    const auto box = std::get<FillRect>(commands.at(0)).rect;
    const auto list = std::get<FillRect>(commands.at(3)).rect;

    EXPECT_EQ(kPanel, std::get<FillRect>(commands.at(3)).color);
    EXPECT_EQ(box.origin.x, list.origin.x);
    EXPECT_EQ(
        box.origin.y + static_cast<std::int32_t>(box.size.height),
        list.origin.y);
    EXPECT_EQ(box.size.width, list.size.width);
}

TEST(DropdownTest, TheOpenListIsPaintedAfterEverythingElse)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);
    ui.button("below");

    const auto texts = textsOf(ui.finish().commands);

    // Declared after the list and drawn before it.
    // That is the whole point of the overlay.
    ASSERT_EQ(5U, texts.size());
    EXPECT_EQ("below", texts.at(2));
    EXPECT_EQ("one", texts.at(3));
    EXPECT_EQ("two", texts.at(4));
}

TEST(DropdownTest, AnOpenListTakesNoRoomFromWhatFollowsIt)
{
    Context closed{kCanvas, plainTheme()};

    closed.dropdown(pickerSpec());
    closed.button("below");

    auto spec = pickerSpec();
    spec.open = true;

    Context open{kCanvas, plainTheme()};

    open.dropdown(spec);
    open.button("below");

    const auto closedCommands = closed.finish().commands;
    const auto openCommands = open.finish().commands;

    EXPECT_EQ(
        std::get<FillRect>(closedCommands.at(3)).rect,
        std::get<FillRect>(openCommands.at(3)).rect);
}

TEST(DropdownTest, PressingAnOptionReportsItsIndex)
{
    auto spec = pickerSpec();
    spec.open = true;

    // The second option.
    // The options sit below the box, each one glyph line tall.
    const Pointer pointer{
        .position = Point{.x = 2, .y = 20}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);

    const auto interactions = ui.finish().interactions;

    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdown = kPicker, .index = 1}),
        *interactions.chosen);
    EXPECT_EQ(WidgetId{101}, interactions.activated);
}

TEST(DropdownTest, HoveringAnOptionLightsUpThatOptionAlone)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{.position = Point{.x = 2, .y = 20}};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);

    const auto frame = ui.finish();

    EXPECT_EQ(WidgetId{101}, frame.interactions.hovered);
    EXPECT_FALSE(frame.interactions.chosen.has_value());

    // The list's own fill, then the first option, then the second.
    EXPECT_EQ(kIdle, std::get<FillRect>(frame.commands.at(4)).color);
    EXPECT_EQ(kHovered, std::get<FillRect>(frame.commands.at(6)).color);
}

TEST(DropdownTest, AnUnnamedOptionStillReportsItsIndex)
{
    auto spec = pickerSpec();
    spec.open = true;
    spec.optionIdBase = kNoWidget;

    const Pointer pointer{
        .position = Point{.x = 2, .y = 20}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);

    const auto interactions = ui.finish().interactions;

    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(1U, interactions.chosen->index);
    EXPECT_EQ(kNoWidget, interactions.activated);
}

TEST(DropdownTest, PressingTheClosedBoxNamesTheDropdownAndNoOption)
{
    const Pointer pointer{
        .position = Point{.x = 2, .y = 2}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(pickerSpec());

    const auto interactions = ui.finish().interactions;

    EXPECT_EQ(kPicker, interactions.activated);
    EXPECT_FALSE(interactions.chosen.has_value());
}

TEST(DropdownTest, TheOpenListIsHitBeforeWhateverItCovers)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{
        .position = Point{.x = 2, .y = 10}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);
    ui.button("below", {.id = kBelow});

    const auto interactions = ui.finish().interactions;

    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(0U, interactions.chosen->index);
    EXPECT_EQ(WidgetId{100}, interactions.activated);
}

TEST(DropdownTest, AnOpenListWithNoOptionsDrawsJustItsPanel)
{
    DropdownSpec spec{.id = kPicker, .placeholder = "pick", .open = true};

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);

    const auto commands = ui.finish().commands;

    EXPECT_EQ(2U, textsOf(commands).size());
    EXPECT_EQ(kPanel, std::get<FillRect>(commands.back()).color);
}
