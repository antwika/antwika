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
#include "antwika/ui/OccluderRects.hpp"
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
using antwika::ui::isOccluded;
using antwika::ui::OptionChoice;
using antwika::ui::Pointer;
using antwika::ui::TextAreaSpec;
using antwika::ui::Theme;
using antwika::ui::WidgetId;

namespace
{
    constexpr Color kPanelColor{.red = 10, .green = 20, .blue = 30};
    constexpr Color kIdleColor{.red = 40, .green = 50, .blue = 60};
    constexpr Color kHoveredColor{.red = 70, .green = 80, .blue = 90};
    constexpr Color kButtonInkColor{.red = 250, .green = 250, .blue = 250};

    constexpr WidgetId kPickerWidget{5};
    constexpr WidgetId kFirstOptionWidget{100};
    constexpr WidgetId kBelowWidget{9};

    constexpr Size kCanvasSize{.width = 200, .height = 100};

    constexpr std::array<std::string_view, 2> kSaves{"one", "two"};

    Theme plainTheme()
    {
        return Theme{
            .panelColor = kPanelColor,
            .buttonIdleColor = kIdleColor,
            .buttonHoveredColor = kHoveredColor,
            .buttonTextColor = kButtonInkColor,
            .textScale = 1,
            .padding = 0,
            .gap = 0,
            .buttonPadding = 0};
    }

    [[nodiscard]] DropdownSpec pickerSpec()
    {
        return DropdownSpec{
            .widgetId = kPickerWidget,
            .optionIdBaseWidget = kFirstOptionWidget,
            .options = kSaves,
            .selectedIndex = 0};
    }

    [[nodiscard]] std::vector<std::string> textsOf(
        const DrawList &drawList)
    {
        std::vector<std::string> texts;

        for (const auto &command : drawList)
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
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(pickerSpec());

    const auto texts = textsOf(uiContext.build().drawList);

    ASSERT_EQ(2U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("v", texts.at(1));
}

TEST(DropdownTest, Dropdown_ClosedWithNothingSelectedItShowsThePlaceholder)
{
    auto spec = pickerSpec();
    spec.selectedIndex = kNoOption;
    spec.placeholder = "pick";

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);

    EXPECT_EQ("pick", textsOf(uiContext.build().drawList).at(0));
}

TEST(DropdownTest, Dropdown_OpenItDrawsEveryOptionUnderTheBox)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);

    const auto commands = uiContext.build().drawList;
    const auto texts = textsOf(commands);

    ASSERT_EQ(4U, texts.size());
    EXPECT_EQ("one", texts.at(0));
    EXPECT_EQ("v", texts.at(1));
    EXPECT_EQ("one", texts.at(2));
    EXPECT_EQ("two", texts.at(3));

    const auto box = std::get<FillRect>(commands.at(0)).rect;
    const auto list = std::get<FillRect>(commands.at(3)).rect;

    EXPECT_EQ(kPanelColor, std::get<FillRect>(commands.at(3)).color);
    EXPECT_EQ(box.originPoint.x, list.originPoint.x);
    EXPECT_EQ(
        box.originPoint.y + static_cast<std::int32_t>(box.size.height),
        list.originPoint.y);
    EXPECT_EQ(box.size.width, list.size.width);
}

TEST(DropdownTest, Dropdown_TheOpenListIsPaintedAfterEverythingElse)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);
    uiContext.button("below");

    const auto texts = textsOf(uiContext.build().drawList);

    ASSERT_EQ(5U, texts.size());
    EXPECT_EQ("below", texts.at(2));
    EXPECT_EQ("one", texts.at(3));
    EXPECT_EQ("two", texts.at(4));
}

TEST(DropdownTest, Dropdown_AnOpenListTakesNoRoomFromWhatFollowsIt)
{
    Context closedContext{kCanvasSize, plainTheme()};

    closedContext.dropdown(pickerSpec());
    closedContext.button("below");

    auto spec = pickerSpec();
    spec.open = true;

    Context openContext{kCanvasSize, plainTheme()};

    openContext.dropdown(spec);
    openContext.button("below");

    const auto closedCommands = closedContext.build().drawList;
    const auto openCommands = openContext.build().drawList;

    EXPECT_EQ(
        std::get<FillRect>(closedCommands.at(3)).rect,
        std::get<FillRect>(openCommands.at(3)).rect);
}

TEST(DropdownTest, Dropdown_PressingAnOptionReportsItsIndex)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 20}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(spec);

    const auto interactions = uiContext.build().interactions;

    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdownWidget = kPickerWidget, .index = 1}),
        *interactions.chosenChoice);
    EXPECT_EQ(WidgetId{101}, interactions.activatedWidget);
}

TEST(DropdownTest, Dropdown_HoveringAnOptionLightsUpThatOptionAlone)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{.positionPoint = Point{.x = 2, .y = 20}};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(spec);

    const auto frame = uiContext.build();

    EXPECT_EQ(WidgetId{101}, frame.interactions.hoveredWidget);
    EXPECT_FALSE(frame.interactions.chosenChoice.has_value());

    EXPECT_EQ(kIdleColor, std::get<FillRect>(frame.drawList.at(4)).color);
    EXPECT_EQ(kHoveredColor, std::get<FillRect>(frame.drawList.at(6)).color);
}

TEST(DropdownTest, Dropdown_AnUnnamedOptionStillReportsItsIndex)
{
    auto spec = pickerSpec();
    spec.open = true;
    spec.optionIdBaseWidget = kNoWidget;

    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 20}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(spec);

    const auto interactions = uiContext.build().interactions;

    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(1U, interactions.chosenChoice->index);
    EXPECT_EQ(kNoWidget, interactions.activatedWidget);
}

TEST(DropdownTest, Dropdown_PressingTheClosedBoxNamesTheDropdownAndNoOption)
{
    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 2}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(pickerSpec());

    const auto interactions = uiContext.build().interactions;

    EXPECT_EQ(kPickerWidget, interactions.activatedWidget);
    EXPECT_FALSE(interactions.chosenChoice.has_value());
}

TEST(DropdownTest, Dropdown_TheOpenListIsHitBeforeWhateverItCovers)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 10}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(spec);
    uiContext.button("below", {.widgetId = kBelowWidget});

    const auto interactions = uiContext.build().interactions;

    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(0U, interactions.chosenChoice->index);
    EXPECT_EQ(WidgetId{100}, interactions.activatedWidget);
}

TEST(DropdownTest, Dropdown_AnUnnamedOptionOverAButtonPressesNoButton)
{
    auto spec = pickerSpec();
    spec.optionIdBaseWidget = kNoWidget;
    spec.open = true;

    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 10}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(spec);
    uiContext.button("below", {.widgetId = kBelowWidget});

    const auto interactions = uiContext.build().interactions;

    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(0U, interactions.chosenChoice->index);
    EXPECT_EQ(kNoWidget, interactions.activatedWidget);
    EXPECT_EQ(kNoWidget, interactions.hoveredWidget);
}

TEST(DropdownTest, Dropdown_AnOptionOverAFocusedAreaMovesNoCaret)
{
    auto spec = pickerSpec();
    spec.open = true;

    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 18}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(spec);
    uiContext.textArea(TextAreaSpec{
        .widgetId = kBelowWidget,
        .widthSizing = antwika::ui::kGrowSizing,
        .heightSizing = antwika::ui::kGrowSizing,
        .text = "abc\ndef\nghi",
        .cursor = 0,
        .focused = true});

    const auto interactions = uiContext.build().interactions;

    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(1U, interactions.chosenChoice->index);
    EXPECT_FALSE(interactions.edit.has_value());
    EXPECT_FALSE(interactions.scrollChange.has_value());
}

TEST(DropdownTest, Dropdown_AnOpenListWithNoOptionsDrawsJustItsPanel)
{
    DropdownSpec spec{.widgetId = kPickerWidget,
        .placeholder = "pick",
        .open = true};

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);

    const auto commands = uiContext.build().drawList;

    EXPECT_EQ(2U, textsOf(commands).size());
    EXPECT_EQ(kPanelColor, std::get<FillRect>(commands.back()).color);
}

TEST(DropdownTest, Dropdown_TheFrontmostOfTwoOverlappingListsAnswers)
{
    auto openSpec = pickerSpec();
    openSpec.open = true;

    auto belowSpec = pickerSpec();
    belowSpec.widgetId = kBelowWidget;
    belowSpec.optionIdBaseWidget = WidgetId{200};
    belowSpec.open = true;

    const Pointer pointer{
        .positionPoint = Point{.x = 2, .y = 18}, .pressed = true};

    Context uiContext{kCanvasSize, plainTheme(), pointer};

    uiContext.dropdown(openSpec);
    uiContext.dropdown(belowSpec);

    const auto interactions = uiContext.build().interactions;

    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdownWidget = kBelowWidget, .index = 0}),
        *interactions.chosenChoice);
    EXPECT_EQ(WidgetId{200}, interactions.activatedWidget);
}

TEST(DropdownTest, Dropdown_TabWalksTheBoxAndThenItsOpenOptions)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::FocusNext, Key::FocusNext}}};

    uiContext.dropdown(spec);

    EXPECT_EQ(WidgetId{100}, uiContext.build().interactions.focusedWidget);
}

TEST(DropdownTest, Dropdown_AFocusedOptionsRingIsDrawnWithTheListItIsIn)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{
        kCanvasSize, plainTheme(), Pointer{}, Keyboard{}, WidgetId{100}};

    uiContext.dropdown(spec);

    const auto commands = uiContext.build().drawList;
    const auto ring = Theme{}.focusRingColor;

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

    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        WidgetId{101}};

    uiContext.dropdown(spec);

    const auto interactions = uiContext.build().interactions;

    EXPECT_EQ(WidgetId{101}, interactions.activatedWidget);
    ASSERT_TRUE(interactions.chosenChoice.has_value());
    EXPECT_EQ(
        (OptionChoice{.dropdownWidget = kPickerWidget, .index = 1}),
        *interactions.chosenChoice);
}

TEST(DropdownTest, Dropdown_EnterOnTheBoxItselfChoosesNothing)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{
        kCanvasSize,
        plainTheme(),
        Pointer{},
        Keyboard{.keys = {Key::Activate}},
        kPickerWidget};

    uiContext.dropdown(spec);

    const auto interactions = uiContext.build().interactions;

    EXPECT_EQ(kPickerWidget, interactions.activatedWidget);
    EXPECT_FALSE(interactions.chosenChoice.has_value());
}

TEST(DropdownTest, Dropdown_ClosedItReportsNoOverlay)
{
    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(pickerSpec());

    EXPECT_TRUE(uiContext.build().overlayRects.empty());
}

TEST(DropdownTest, Dropdown_AnOpenListReportsAnOverlayOverWhatItCovers)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);
    uiContext.button("below", {.widgetId = kBelowWidget});

    const auto frame = uiContext.build();
    const auto foundRect = frame.rects.find(kBelowWidget);

    ASSERT_TRUE(foundRect.has_value());
    ASSERT_FALSE(frame.overlayRects.empty());

    EXPECT_TRUE(isOccluded(
        frame.overlayRects,
        HoverPointer{.positionPoint = foundRect->originPoint}));
}

TEST(DropdownTest, Dropdown_BoxesTheOptionsThatTurnSomethingOn)
{
    constexpr std::array kMarked{
        antwika::ui::OptionMark::Off, antwika::ui::OptionMark::On};

    auto spec = pickerSpec();
    spec.open = true;
    spec.markedOptions = kMarked;

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);

    const auto commands = uiContext.build().drawList;

    EXPECT_EQ(4U, textsOf(commands).size());

    auto boxes = 0;

    for (const auto &command : commands)
    {
        const auto *fill = std::get_if<FillRect>(&command);

        if (fill != nullptr
            && fill->rect.size.width == plainTheme().checkboxSize
            && fill->rect.size.height == plainTheme().checkboxSize)
        {
            ++boxes;
        }
    }

    EXPECT_EQ(2, boxes);
}

TEST(DropdownTest, Dropdown_KeepsNoMarkColumnWhereNoneIsAskedFor)
{
    auto spec = pickerSpec();
    spec.open = true;

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);

    EXPECT_EQ(4U, textsOf(uiContext.build().drawList).size());
}

TEST(DropdownTest, Dropdown_LeavesOptionsItWasToldNothingOfBare)
{
    constexpr std::array kMarked{antwika::ui::OptionMark::On};

    auto spec = pickerSpec();
    spec.open = true;
    spec.markedOptions = kMarked;

    Context uiContext{kCanvasSize, plainTheme()};

    uiContext.dropdown(spec);

    auto boxes = 0;

    for (const auto &command : uiContext.build().drawList)
    {
        const auto *fill = std::get_if<FillRect>(&command);

        if (fill != nullptr
            && fill->rect.size.width == plainTheme().checkboxSize)
        {
            ++boxes;
        }
    }

    EXPECT_EQ(1, boxes);
}
