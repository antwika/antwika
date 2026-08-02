#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <variant>

#include <gtest/gtest.h>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/ui_demo/DemoScene.hpp"
#include "antwika/ui_demo/DemoState.hpp"
#include "antwika/ui_demo/MessageId.hpp"
#include "antwika/ui_demo/Messages.hpp"
#include "antwika/ui_demo/Showcase.hpp"
#include "antwika/ui_demo/Widgets.hpp"
#include "WidgetCentre.hpp"

using antwika::gfx::Color;
using antwika::gfx::Point;
using antwika::gfx::Size;
using antwika::ui::DrawText;
using antwika::ui::FillRect;
using antwika::ui::Frame;
using antwika::ui::Keyboard;
using antwika::ui::Pointer;
using antwika::ui_demo::DemoScene;
using antwika::ui_demo::DemoState;
using antwika::ui_demo::kShowcaseCount;
using antwika::ui_demo::Showcase;
using antwika::ui_demo::tests::widgetCentre;
namespace widgets = antwika::ui_demo::widgets;

namespace
{
    // The locale is a constant of the build, so a test may name one.
    // Every case here asserts the English showcase's own layout.
    constexpr antwika::ui_demo::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    constexpr Size kCanvas{.width = 960, .height = 720};

    // What DemoScene paints its corner lamp in, either way.
    constexpr Color kLampOn{.red = 120, .green = 230, .blue = 170};
    constexpr Color kLampOff{.red = 40, .green = 44, .blue = 52};

    // The first accent, and the theme's ordinary text.
    constexpr Color kAmber{.red = 244, .green = 180, .blue = 60};
    constexpr Color kText{.red = 232, .green = 236, .blue = 232};

    // One line only this page draws, per page, in Showcase order.
    constexpr std::array<std::string_view, kShowcaseCount> kMarkers{
        "a growing spacer",
        "count",
        "start",
        "tab here and type",
        "none chosen",
        "first",
        "focusRing",
        "this row is named",
        "too little room shrinks children in proportion",
        "line 0"};

    [[nodiscard]] std::optional<DrawText> findText(
        const Frame &frame, const std::string_view text)
    {
        for (const auto &command : frame.commands)
        {
            const auto *drawn = std::get_if<DrawText>(&command);

            if (drawn != nullptr && drawn->text == text)
            {
                return *drawn;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] Frame describePage(
        const Showcase page,
        const Pointer pointer = {},
        const Keyboard &keyboard = {})
    {
        DemoState state;
        state.select(static_cast<std::size_t>(page));

        const DemoScene scene{kTranslator};
        return scene.describe(kCanvas, pointer, keyboard, state);
    }

    TEST(DemoSceneTest, Describe_ShowsThePageThePickerSelected)
    {
        for (std::size_t index = 0; index < kShowcaseCount; ++index)
        {
            const auto frame = describePage(
                static_cast<Showcase>(index));

            EXPECT_TRUE(findText(frame, kMarkers[index]).has_value())
                << "page " << index << " drew no " << kMarkers[index];

            for (std::size_t other = 0; other < kShowcaseCount; ++other)
            {
                if (other == index)
                {
                    continue;
                }

                EXPECT_FALSE(findText(frame, kMarkers[other]).has_value())
                    << "page " << index << " drew " << kMarkers[other];
            }
        }
    }

    TEST(DemoSceneTest, Describe_IsAPureFunctionOfWhatItIsHanded)
    {
        const auto once = describePage(Showcase::Buttons);
        const auto again = describePage(Showcase::Buttons);

        EXPECT_EQ(once.commands, again.commands);
        EXPECT_EQ(once.rects, again.rects);
        EXPECT_EQ(once.interactions, again.interactions);
    }

    TEST(DemoSceneTest, Describe_NamesThePickerAndThePageOnEveryPage)
    {
        for (std::size_t index = 0; index < kShowcaseCount; ++index)
        {
            const auto frame = describePage(
                static_cast<Showcase>(index));

            EXPECT_TRUE(frame.rects.find(widgets::kPicker).has_value());
            EXPECT_TRUE(frame.rects.find(widgets::kCard).has_value());
        }
    }

    TEST(DemoSceneTest, Describe_ListsThePagesOnceThePickerIsOpen)
    {
        DemoState state;
        state.setPickerOpen(true);

        const DemoScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, {}, state);

        for (std::size_t index = 0; index < kShowcaseCount; ++index)
        {
            const std::string option = kTranslator.text(
                showcaseNameId(static_cast<Showcase>(index)));

            EXPECT_TRUE(findText(frame, option).has_value())
                << "no option named " << option;
        }
    }

    TEST(DemoSceneTest, Describe_PlacesTheMarkerFromTheNamedRowsRect)
    {
        const auto frame = describePage(Showcase::Rects);

        const auto row = frame.rects.find(widgets::kMarked);
        ASSERT_TRUE(row.has_value());

        // Last is the lamp, and the marker is the command before it.
        ASSERT_GE(frame.commands.size(), 2U);
        const auto *marker = std::get_if<FillRect>(
            &frame.commands[frame.commands.size() - 2]);
        ASSERT_NE(marker, nullptr);

        EXPECT_EQ(marker->rect.origin.x, row->origin.x);
        EXPECT_EQ(marker->rect.size.width, row->size.width);
        EXPECT_EQ(
            marker->rect.origin.y,
            row->origin.y + static_cast<std::int32_t>(row->size.height));
    }

    TEST(DemoSceneTest, Describe_MarksNothingForAnIdNoPageDeclares)
    {
        const auto frame = describePage(Showcase::Labels);

        EXPECT_FALSE(
            frame.rects.find(widgets::kNeverDeclared).has_value());
        EXPECT_FALSE(frame.rects.find(widgets::kMarked).has_value());
    }

    TEST(DemoSceneTest, Describe_LightsTheCornerWhileThePointerIsOnIt)
    {
        const auto dark = describePage(Showcase::Labels);
        const auto *unlit = std::get_if<FillRect>(&dark.commands.back());
        ASSERT_NE(unlit, nullptr);
        EXPECT_EQ(unlit->color, kLampOff);

        DemoState state;
        const DemoScene scene{kTranslator};
        const auto found = scene.describe(kCanvas, {}, {}, state);
        const auto over = widgetCentre(found, widgets::kCard);
        ASSERT_TRUE(over.has_value());

        const auto frame = scene.describe(
            kCanvas, Pointer{.position = over}, {}, state);
        const auto *lit = std::get_if<FillRect>(&frame.commands.back());
        ASSERT_NE(lit, nullptr);
        EXPECT_EQ(lit->color, kLampOn);
    }

    TEST(DemoSceneTest, Describe_PaintsTheDropdownsLineInTheAccent)
    {
        constexpr std::string_view kLine =
            "an open list is an overlay, hit first";

        DemoState state;
        state.select(static_cast<std::size_t>(Showcase::Dropdown));

        const DemoScene scene{kTranslator};

        const auto plain = scene.describe(kCanvas, {}, {}, state);
        const auto before = findText(plain, kLine);
        ASSERT_TRUE(before.has_value());
        EXPECT_EQ(before->color, kText);

        state.selectAccent(0);
        const auto accented = scene.describe(kCanvas, {}, {}, state);
        const auto after = findText(accented, kLine);
        ASSERT_TRUE(after.has_value());
        EXPECT_EQ(after->color, kAmber);
    }

    TEST(DemoSceneTest, Describe_ShrinksChildrenIntoTheRoomTheyHave)
    {
        const auto frame = describePage(Showcase::Shrink);

        const auto strip = frame.rects.find(widgets::kSqueezed);
        ASSERT_TRUE(strip.has_value());
        EXPECT_EQ(strip->size.width, 180U);
    }

    TEST(DemoSceneTest, Describe_ReportsTheFocusItWasHanded)
    {
        DemoState state;
        state.select(static_cast<std::size_t>(Showcase::Focus));
        state.setFocus(widgets::kSecond);

        const DemoScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, {}, state);

        EXPECT_EQ(frame.interactions.focused, widgets::kSecond);
    }

    TEST(DemoSceneTest, Describe_DrawsTheFieldsCharactersAndItsCaret)
    {
        DemoState state;
        state.select(static_cast<std::size_t>(Showcase::TextField));
        state.setText("hi", 1);
        state.setFocus(widgets::kField);

        const DemoScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, {}, state);

        EXPECT_TRUE(findText(frame, "h").has_value());
        EXPECT_TRUE(findText(frame, "i").has_value());
        EXPECT_TRUE(findText(frame, "holding: hi").has_value());
    }

    // The message a page reports is an id in the state and words here.
    // Its argument is a datum the tick path produced.
    TEST(DemoSceneTest, Describe_WordsAMessageCarryingAPlainDatum)
    {
        DemoState state;
        state.setMessage(
            {.id = antwika::ui_demo::MessageId::Submitted,
             .datum = "ok",
             .argId = std::nullopt});

        const DemoScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, {}, state);

        EXPECT_TRUE(findText(frame, "submitted: ok").has_value());
    }

    // Or the argument is itself something the catalogue words.
    // Which is the whole reason DemoMessage has two argument fields.
    TEST(DemoSceneTest, Describe_WordsAMessageWhoseArgumentIsAnotherId)
    {
        DemoState state;
        state.setMessage(
            {.id = antwika::ui_demo::MessageId::Showing,
             .datum = {},
             .argId = antwika::ui_demo::MessageId::PageShrink});

        const DemoScene scene{kTranslator};
        const auto frame = scene.describe(kCanvas, {}, {}, state);

        EXPECT_TRUE(findText(frame, "showing shrink").has_value());
    }

    // Every word the showcase declares is a lookup.
    // The eleven ui::Theme swatch labels deliberately are not.
    // Those are that struct's own field names.
    // A translated one would lie about what a caller types.
    TEST(DemoSceneTest, Describe_IsWordedByWhicheverTranslatorItHolds)
    {
        constexpr antwika::ui_demo::Translator swedish{
            antwika::i18n::Locale::Swedish};

        DemoState state;
        state.select(static_cast<std::size_t>(Showcase::Theme));

        const DemoScene scene{swedish};
        const auto frame = scene.describe(kCanvas, {}, {}, state);

        EXPECT_TRUE(
            findText(frame, "antwika::ui-uppvisning").has_value());
        EXPECT_TRUE(findText(frame, "focusRing").has_value());
        EXPECT_FALSE(
            findText(frame, "antwika::ui showcase").has_value());
    }

    // A button is as wide as its own label, so the layout moves.
    // Which is exactly why the locale is fixed for a run.
    TEST(DemoSceneTest, Describe_LaysOutFromTheWordsItWasGiven)
    {
        constexpr antwika::ui_demo::Translator swedish{
            antwika::i18n::Locale::Swedish};

        DemoState state;
        state.select(static_cast<std::size_t>(Showcase::Buttons));

        const auto english =
            DemoScene{kTranslator}.describe(kCanvas, {}, {}, state);
        const auto other =
            DemoScene{swedish}.describe(kCanvas, {}, {}, state);

        const auto here = english.rects.find(widgets::kCount);
        const auto there = other.rects.find(widgets::kCount);

        ASSERT_TRUE(here.has_value());
        ASSERT_TRUE(there.has_value());
        EXPECT_NE(here->size.width, there->size.width);
    }
} // namespace
