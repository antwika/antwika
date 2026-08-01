#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::describeEditor;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::statusLine;
using antwika::i18n::MessageId;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::gfx::Point;
using antwika::gfx::Rect;
using antwika::gfx::Size;
using antwika::ui::kNoWidget;
using antwika::ui::Pointer;
using antwika::ui::WidgetId;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 480};
    constexpr Size kSheet{.width = 32, .height = 16};

    EditorState opened()
    {
        return EditorState{
            Canvas::blank(kSheet),
            TileGrid{.width = 16, .height = 8},
            kCanvas};
    }

    Point middleOf(const Rect &rect)
    {
        return Point{
            .x = rect.origin.x
                 + static_cast<std::int32_t>(rect.size.width / 2),
            .y = rect.origin.y
                 + static_cast<std::int32_t>(rect.size.height / 2)};
    }

    // The locale is a constant of the build, so a test may name one.
    // Every case here asserts the English bar's own layout.
    constexpr antwika::i18n::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    // Press the middle of the widget's own rectangle.
    // That is the one place that cannot drift from the layout.
    WidgetId pressOn(const EditorState &state, const WidgetId widget)
    {
        const auto rect =
            describeEditor(state, Pointer{}, kTranslator)
                .rects.find(widget);

        if (!rect.has_value())
        {
            return kNoWidget;
        }

        return describeEditor(
                   state,
                   Pointer{
                       .position = middleOf(*rect),
                       .down = true,
                       .pressed = true},
                   kTranslator)
            .interactions.activated;
    }
} // namespace

TEST(EditorUiTest, DescribeEditor_DrawsSomethingAndNamesEveryWidget)
{
    const EditorState state = opened();
    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_FALSE(frame.commands.empty());

    namespace widgets = antwika::atlas_editor::widgets;
    EXPECT_TRUE(frame.rects.find(widgets::kSave).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kLoad).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kGrid).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kZoomIn).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kZoomOut).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kResetView).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::toolWidget(Tool::Pick)).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::swatchWidget(0)).has_value());
}

TEST(EditorUiTest, DescribeEditor_GivesEverySwatchAnAreaToClick)
{
    const EditorState state = opened();
    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    const auto rect = frame.rects.find(widgets::swatchWidget(2));

    ASSERT_TRUE(rect.has_value());
    EXPECT_GT(rect->size.width, 0U);
    EXPECT_GT(rect->size.height, 0U);
}

TEST(EditorUiTest, DescribeEditor_ReportsAPressOnTheWidgetItLandedOn)
{
    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    EXPECT_EQ(pressOn(state, widgets::kSave), widgets::kSave);
    EXPECT_EQ(
        pressOn(state, widgets::swatchWidget(4)),
        widgets::swatchWidget(4));
    EXPECT_EQ(
        pressOn(state, widgets::toolWidget(Tool::Erase)),
        widgets::toolWidget(Tool::Erase));
}

TEST(EditorUiTest, DescribeEditor_ReportsThePointerIsOffTheBarBelowIt)
{
    const EditorState state = opened();

    const auto frame = describeEditor(
        state,
        Pointer{
            .position = Point{
                .x = 400,
                .y = static_cast<std::int32_t>(kCanvas.height - 1)},
            .down = false,
            .pressed = false},
        kTranslator);

    EXPECT_FALSE(frame.interactions.pointerOverUi);
    EXPECT_EQ(frame.interactions.activated, kNoWidget);
}

TEST(EditorUiTest, DescribeEditor_ReportsThePointerIsOnTheBar)
{
    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    const auto rect =
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::kSave);
    ASSERT_TRUE(rect.has_value());

    const auto frame = describeEditor(
        state, Pointer{.position = middleOf(*rect)}, kTranslator);

    EXPECT_TRUE(frame.interactions.pointerOverUi);
}

TEST(StatusLineTest, StatusLine_SaysWhatWouldHappenAndWhereItWould)
{
    EditorState state = opened();
    state.moveTo(Point{
        .x = state.view().pan.x + 17, .y = state.view().pan.y + 9});

    const std::string line = statusLine(state, kTranslator);

    EXPECT_NE(line.find("PAINT"), std::string::npos);
    EXPECT_NE(line.find("px 17,9"), std::string::npos);

    // Sixteen by eight slots on a thirty-two by sixteen sheet.
    // The pixel at (17, 9) is in the last of the four.
    EXPECT_NE(line.find("slot 3"), std::string::npos);
    EXPECT_NE(line.find("32x16"), std::string::npos);
    EXPECT_EQ(line.find("UNSAVED"), std::string::npos);
}

TEST(StatusLineTest, StatusLine_SaysNothingIsUnderThePointerUntilItIs)
{
    const EditorState state = opened();

    EXPECT_NE(statusLine(state, kTranslator).find("px -,-"), std::string::npos);
}

TEST(StatusLineTest, StatusLine_SaysWhenAPixelIsInNoSlotAtAll)
{
    EditorState state = opened();
    state.moveTo(Point{.x = 0, .y = 0});

    EXPECT_NE(
        statusLine(state, kTranslator).find("slot -"),
        std::string::npos);
}

TEST(StatusLineTest, StatusLine_SaysWhenThereIsSomethingToSave)
{
    EditorState state = opened();
    state.applyAt(Point{
        .x = state.view().pan.x + 1, .y = state.view().pan.y + 1});
    state.setStatus(
        {.id = MessageId::AtlasSaveFailed,
         .detail = "nowhere to write"});

    const std::string line = statusLine(state, kTranslator);

    EXPECT_NE(line.find("UNSAVED"), std::string::npos);
    EXPECT_NE(line.find("save failed"), std::string::npos);
}

// Every word on the bar and in the line under it is a lookup.
// The one thing left as notation is the slot dash and the "px".
// A detail carried by a status is never translated either.
// It is a path or a failure's own words, which is a diagnostic.
TEST(StatusLineTest, StatusLine_IsWordedByWhicheverTranslatorItIsGiven)
{
    constexpr antwika::i18n::Translator swedish{
        antwika::i18n::Locale::Swedish};

    EditorState state = opened();
    state.setStatus(
        {.id = MessageId::AtlasSaveFailed,
         .detail = "nowhere to write"});

    const std::string line = statusLine(state, swedish);

    EXPECT_NE(line.find("ÅLA"), std::string::npos);
    EXPECT_NE(line.find("kunde inte spara"), std::string::npos);
    EXPECT_NE(line.find("nowhere to write"), std::string::npos);
    EXPECT_EQ(line.find("PAINT"), std::string::npos);
}

// The bar is measured from the words, which is the whole cost.
// A language is therefore fixed for a run rather than chosen in it.
// This is what would go wrong if it were not.
TEST(EditorUiTest, DescribeEditor_LaysTheBarOutFromTheWordsItIsGiven)
{
    constexpr antwika::i18n::Translator swedish{
        antwika::i18n::Locale::Swedish};

    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    const auto english =
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::kSave);
    const auto other = describeEditor(state, Pointer{}, swedish)
                           .rects.find(widgets::kSave);

    ASSERT_TRUE(english.has_value());
    ASSERT_TRUE(other.has_value());
    EXPECT_NE(english->size.width, other->size.width);
}
