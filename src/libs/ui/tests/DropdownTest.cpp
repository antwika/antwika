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
#include "antwika/ui/HoverPointer.hpp"
#include "antwika/ui/Keyboard.hpp"
#include "antwika/ui/Overlays.hpp"
#include "antwika/ui/Pointer.hpp"
#include "antwika/ui/Sizing.hpp"
#include "antwika/ui/TextAreaSpec.hpp"
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
using antwika::ui::HoverPointer;
using antwika::ui::Key;
using antwika::ui::Keyboard;
using antwika::ui::kNoOption;
using antwika::ui::kNoWidget;
using antwika::ui::overlaid;
using antwika::ui::OptionChoice;
using antwika::ui::Pointer;
using antwika::ui::TextAreaSpec;
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
}

TEST(DropdownTest, Dropdown_ClosedItShowsOnlyWhatIsSelected)
{
    Context ui{kCanvas, plainTheme()};

    ui.dropdown(pickerSpec());

    const auto texts = textsOf(ui.finish().commands);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("v", texts.at(1));
}

TEST(DropdownTest, Dropdown_ClosedWithNothingSelectedItShowsThePlaceholder)
{
    auto spec = pickerSpec();
    spec.selected = kNoOption;
    spec.placeholder = "pick";

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);

    EXPECT_EQ("pick", textsOf(ui.finish().commands).at(0));
}

TEST(DropdownTest, Dropdown_OpenItDrawsEveryOptionUnderTheBox)
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

    const auto box = std::get<FillRect>(commands.at(0)).rect;
    const auto list = std::get<FillRect>(commands.at(3)).rect;

    EXPECT_EQ(kPanel, std::get<FillRect>(commands.at(3)).color);
    EXPECT_EQ(box.origin.x, list.origin.x);
    EXPECT_EQ(
        box.origin.y + static_cast<std::int32_t>(box.size.height),
        list.origin.y);
    EXPECT_EQ(box.size.width, list.size.width);
}

TEST(DropdownTest, Dropdown_TheOpenListIsPaintedAfterEverythingElse)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);
    ui.button("below");

    const auto texts = textsOf(ui.finish().commands);

    ASSERT_EQ(5U, texts.size());
    EXPECT_EQ("below", texts.at(2));
    EXPECT_EQ("one", texts.at(3));
    EXPECT_EQ("two", texts.at(4));
}

TEST(DropdownTest, Dropdown_AnOpenListTakesNoRoomFromWhatFollowsIt)
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

TEST(DropdownTest, Dropdown_PressingAnOptionReportsItsIndex)
{
    auto spec = pickerSpec();
    spec.open = true;

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

TEST(DropdownTest, Dropdown_HoveringAnOptionLightsUpThatOptionAlone)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{.position = Point{.x = 2, .y = 20}};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);

    const auto frame = ui.finish();

    EXPECT_EQ(WidgetId{101}, frame.interactions.hovered);
    EXPECT_FALSE(frame.interactions.chosen.has_value());

    EXPECT_EQ(kIdle, std::get<FillRect>(frame.commands.at(4)).color);
    EXPECT_EQ(kHovered, std::get<FillRect>(frame.commands.at(6)).color);
}

TEST(DropdownTest, Dropdown_AnUnnamedOptionStillReportsItsIndex)
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

TEST(DropdownTest, Dropdown_PressingTheClosedBoxNamesTheDropdownAndNoOption)
{
    const Pointer pointer{
        .position = Point{.x = 2, .y = 2}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(pickerSpec());

    const auto interactions = ui.finish().interactions;

    EXPECT_EQ(kPicker, interactions.activated);
    EXPECT_FALSE(interactions.chosen.has_value());
}

TEST(DropdownTest, Dropdown_TheOpenListIsHitBeforeWhateverItCovers)
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

TEST(DropdownTest, Dropdown_AnUnnamedOptionOverAButtonPressesNoButton)
{
    auto spec = pickerSpec();
    spec.optionIdBase = kNoWidget;
    spec.open = true;

    const Pointer pointer{
        .position = Point{.x = 2, .y = 10}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);
    ui.button("below", {.id = kBelow});

    const auto interactions = ui.finish().interactions;

    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(0U, interactions.chosen->index);
    EXPECT_EQ(kNoWidget, interactions.activated);
    EXPECT_EQ(kNoWidget, interactions.hovered);
}

TEST(DropdownTest, Dropdown_AnOptionOverAFocusedAreaMovesNoCaret)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{
        .position = Point{.x = 2, .y = 18}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(spec);
    ui.textArea(TextAreaSpec{
        .id = kBelow,
        .width = antwika::ui::kGrow,
        .height = antwika::ui::kGrow,
        .text = "abc\ndef\nghi",
        .cursor = 0,
        .focused = true});

    const auto interactions = ui.finish().interactions;

    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(1U, interactions.chosen->index);
    EXPECT_FALSE(interactions.edit.has_value());
    EXPECT_FALSE(interactions.scrolled.has_value());
}

TEST(DropdownTest, Dropdown_AnOpenListWithNoOptionsDrawsJustItsPanel)
{
    DropdownSpec spec{.id = kPicker, .placeholder = "pick", .open = true};

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);

    const auto commands = ui.finish().commands;

    EXPECT_EQ(2U, textsOf(commands).size());
    EXPECT_EQ(kPanel, std::get<FillRect>(commands.back()).color);
}

TEST(DropdownTest, Dropdown_TheFrontmostOfTwoOverlappingListsAnswers)
{
    auto above = pickerSpec();
    above.open = true;

    auto below = pickerSpec();
    below.id = kBelow;
    below.optionIdBase = WidgetId{200};
    below.open = true;

    const Pointer pointer{
        .position = Point{.x = 2, .y = 18}, .pressed = true};

    Context ui{kCanvas, plainTheme(), pointer};

    ui.dropdown(above);
    ui.dropdown(below);

    const auto interactions = ui.finish().interactions;

    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdown = kBelow, .index = 0}),
        *interactions.chosen);
    EXPECT_EQ(WidgetId{200}, interactions.activated);
}

TEST(DropdownTest, Dropdown_TabWalksTheBoxAndThenItsOpenOptions)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext, Key::FocusNext}}};

    ui.dropdown(spec);

    EXPECT_EQ(WidgetId{100}, ui.finish().interactions.focused);
}

TEST(DropdownTest, Dropdown_AFocusedOptionsRingIsDrawnWithTheListItIsIn)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{
        kCanvas, plainTheme(), Pointer{}, Keyboard{}, WidgetId{100}};

    ui.dropdown(spec);

    const auto commands = ui.finish().commands;
    const auto ring = Theme{}.focusRing;

    ASSERT_GE(commands.size(), 4U);

    for (auto index = commands.size() - 4; index < commands.size();
         ++index)
    {
        EXPECT_EQ(ring, std::get<FillRect>(commands.at(index)).color);
    }
}

TEST(DropdownTest, Dropdown_EnterChoosesTheOptionItIsOn)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        WidgetId{101}};

    ui.dropdown(spec);

    const auto interactions = ui.finish().interactions;

    EXPECT_EQ(WidgetId{101}, interactions.activated);
    ASSERT_TRUE(interactions.chosen.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdown = kPicker, .index = 1}),
        *interactions.chosen);
}

TEST(DropdownTest, Dropdown_EnterOnTheBoxItselfChoosesNothing)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{
        kCanvas,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kPicker};

    ui.dropdown(spec);

    const auto interactions = ui.finish().interactions;

    EXPECT_EQ(kPicker, interactions.activated);
    EXPECT_FALSE(interactions.chosen.has_value());
}

TEST(DropdownTest, Dropdown_ClosedItReportsNoOverlay)
{
    Context ui{kCanvas, plainTheme()};

    ui.dropdown(pickerSpec());

    EXPECT_TRUE(ui.finish().overlays.empty());
}

TEST(DropdownTest, Dropdown_AnOpenListReportsAnOverlayOverWhatItCovers)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context ui{kCanvas, plainTheme()};

    ui.dropdown(spec);
    ui.button("below", {.id = kBelow});

    const auto frame = ui.finish();
    const auto covered = frame.rects.find(kBelow);

    ASSERT_TRUE(covered.has_value());
    ASSERT_FALSE(frame.overlays.empty());

    EXPECT_TRUE(overlaid(
        frame.overlays,
        HoverPointer{.position = covered->origin}));
}
