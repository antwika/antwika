#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <variant>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/DrawCommand.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/HoverPointer.hpp>
#include <antwika/ui/Overlays.hpp>
#include <antwika/ui/Pointer.hpp>

#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorTheme.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/FileList.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::AtlasField;
using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::describeEditor;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::entryText;
using antwika::atlas_editor::FileEntry;
using antwika::atlas_editor::fieldNameId;
using antwika::atlas_editor::filesShownIn;
using antwika::atlas_editor::kAtlasFieldCount;
using antwika::atlas_editor::kAtlasPresetCount;
using antwika::atlas_editor::presetNameId;
using antwika::atlas_editor::kCardLabels;
using antwika::atlas_editor::MessageId;
using antwika::atlas_editor::metaLines;
using antwika::atlas_editor::statusLine;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::atlas_editor::toolNameId;
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

    constexpr Size kWindow{.width = 1280, .height = 720};
    constexpr Size kWideWindow{.width = 1920, .height = 1080};

    const std::size_t kFileRows = filesShownIn(kCanvas, kCardLabels);

    std::vector<FileEntry> manyFiles(const std::size_t count)
    {
        std::vector<FileEntry> listed;

        for (std::size_t at = 0; at < count; ++at)
        {
            FileEntry entry;
            entry.name = std::to_string(at) + ".png";

            listed.push_back(std::move(entry));
        }

        return listed;
    }

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

    constexpr antwika::atlas_editor::Translator kTranslator{
        antwika::i18n::kDefaultLocale};

    std::optional<Point> tipFor(
        const antwika::ui::Frame &frame, const std::string &text)
    {
        for (const auto &command : frame.commands)
        {
            const auto *drawn =
                std::get_if<antwika::ui::DrawText>(&command);

            if (drawn != nullptr && drawn->text == text)
            {
                return drawn->origin;
            }
        }

        return std::nullopt;
    }

    [[nodiscard]] bool drew(
        const antwika::ui::Frame &frame, const std::string &text)
    {
        return tipFor(frame, text).has_value();
    }

    [[nodiscard]] bool namesEveryFile(const Size canvas)
    {
        EditorState state{
            Canvas::blank(kSheet),
            TileGrid{.width = 16, .height = 8},
            canvas};

        state.showModal(
            antwika::atlas_editor::Modal::Load, "..", manyFiles(40));

        const auto frame = describeEditor(state, Pointer{}, kTranslator);

        const auto shown = filesShownIn(canvas, kCardLabels);

        for (std::size_t at = 0; at < shown; ++at)
        {
            if (!drew(frame, entryText(state.files()[at])))
            {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] bool holds(const Rect &rect, const Point point)
    {
        const auto right =
            rect.origin.x + static_cast<std::int32_t>(rect.size.width);
        const auto bottom =
            rect.origin.y + static_cast<std::int32_t>(rect.size.height);

        return point.x >= rect.origin.x && point.x < right
               && point.y >= rect.origin.y && point.y < bottom;
    }

    std::optional<Tool> toolUnder(
        const antwika::ui::Frame &frame, const Point where)
    {
        namespace widgets = antwika::atlas_editor::widgets;

        for (std::size_t index = 0;
             index < antwika::atlas_editor::kToolCount;
             ++index)
        {
            const auto tool = static_cast<Tool>(index);
            const auto rect =
                frame.rects.find(widgets::toolWidget(tool));

            if (rect.has_value() && holds(*rect, where))
            {
                return tool;
            }
        }

        return std::nullopt;
    }

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
}

TEST(EditorUiTest, DescribeEditor_DrawsSomethingAndNamesEveryWidget)
{
    const EditorState state = opened();
    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_FALSE(frame.commands.empty());

    namespace widgets = antwika::atlas_editor::widgets;
    EXPECT_TRUE(frame.rects.find(widgets::kFileMenu).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kViewMenu).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kStatusBar).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kToolRail).has_value());
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
    EXPECT_EQ(
        pressOn(state, widgets::kFileMenu), widgets::kFileMenu);
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
                .x = static_cast<std::int32_t>(kCanvas.width / 2),
                .y = static_cast<std::int32_t>(kCanvas.height / 2)},
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
            .rects.find(widgets::kFileMenu);
    ASSERT_TRUE(rect.has_value());

    const auto frame = describeEditor(
        state, Pointer{.position = middleOf(*rect)}, kTranslator);

    EXPECT_TRUE(frame.interactions.pointerOverUi);
}

TEST(EditorUiTest, DescribeEditor_RailsEveryToolDownTheLeftEdge)
{
    const EditorState state = opened();
    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    const auto rail = frame.rects.find(widgets::kToolRail);
    ASSERT_TRUE(rail.has_value());

    EXPECT_EQ(rail->origin.x, 0);
    EXPECT_LT(rail->size.width, kCanvas.width / 4);

    for (std::size_t index = 0;
         index < antwika::atlas_editor::kToolCount;
         ++index)
    {
        const auto tool =
            static_cast<antwika::atlas_editor::Tool>(index);
        const auto button = frame.rects.find(widgets::toolWidget(tool));

        ASSERT_TRUE(button.has_value()) << index;
        EXPECT_GE(button->origin.x, rail->origin.x) << index;
        EXPECT_LE(
            button->origin.x
                + static_cast<std::int32_t>(button->size.width),
            rail->origin.x
                + static_cast<std::int32_t>(rail->size.width))
            << index;
    }
}

TEST(EditorUiTest, DescribeEditor_FootsTheDrawingAreaWithTheStatusBar)
{
    const EditorState state = opened();
    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    namespace widgets = antwika::atlas_editor::widgets;
    const auto foot = frame.rects.find(widgets::kStatusBar);
    const auto rail = frame.rects.find(widgets::kToolRail);
    ASSERT_TRUE(foot.has_value());
    ASSERT_TRUE(rail.has_value());

    EXPECT_EQ(
        foot->origin.y + static_cast<std::int32_t>(foot->size.height),
        static_cast<std::int32_t>(kCanvas.height));
    EXPECT_GE(
        foot->origin.y,
        rail->origin.y + static_cast<std::int32_t>(rail->size.height));
}

TEST(EditorUiTest, DescribeEditor_ShowsAMenusItemsOnlyWhileItIsOpen)
{
    namespace widgets = antwika::atlas_editor::widgets;
    using antwika::atlas_editor::FileItem;

    EditorState state = opened();

    EXPECT_FALSE(
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::fileItemWidget(FileItem::Quit))
            .has_value());

    state.showMenu(antwika::atlas_editor::Menu::File);

    EXPECT_TRUE(
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::fileItemWidget(FileItem::Quit))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesTheToolThePointerRestsOn)
{
    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    const auto button =
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::toolWidget(Tool::Ellipse));
    ASSERT_TRUE(button.has_value());

    const auto frame = describeEditor(
        state, Pointer{.position = middleOf(*button)}, kTranslator);

    const auto named = tipFor(frame, kTranslator.text(MessageId::ToolEllipse));

    ASSERT_TRUE(named.has_value());
    EXPECT_GE(
        named->x,
        button->origin.x
            + static_cast<std::int32_t>(button->size.width));
}

TEST(EditorUiTest, DescribeEditor_NamesNoToolWithThePointerOffTheRail)
{
    const EditorState state = opened();

    const auto frame = describeEditor(
        state,
        Pointer{.position = Point{.x = 400, .y = 400}},
        kTranslator);

    EXPECT_FALSE(
        tipFor(frame, kTranslator.text(MessageId::ToolEllipse))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesNoToolFromTheRailsOwnMargin)
{
    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    const auto button =
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::toolWidget(Tool::Paint));
    ASSERT_TRUE(button.has_value());
    ASSERT_GT(button->origin.x, 0);

    const auto frame = describeEditor(
        state,
        Pointer{
            .position = Point{
                .x = button->origin.x - 1,
                .y = middleOf(*button).y}},
        kTranslator);

    EXPECT_FALSE(
        tipFor(frame, kTranslator.text(MessageId::ToolPaint))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesNoToolFromAboveTheFirstButton)
{
    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    const auto shown = describeEditor(state, Pointer{}, kTranslator);
    const auto rail = shown.rects.find(widgets::kToolRail);
    const auto button =
        shown.rects.find(widgets::toolWidget(Tool::Paint));
    ASSERT_TRUE(rail.has_value());
    ASSERT_TRUE(button.has_value());
    ASSERT_GT(button->origin.y, rail->origin.y);

    const auto frame = describeEditor(
        state,
        Pointer{
            .position = Point{
                .x = middleOf(*button).x, .y = rail->origin.y}},
        kTranslator);

    EXPECT_FALSE(
        tipFor(frame, kTranslator.text(MessageId::ToolPaint))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesNoToolWithNoPointerAtAll)
{
    const EditorState state = opened();
    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_FALSE(
        tipFor(frame, kTranslator.text(MessageId::ToolPaint))
            .has_value());
}

TEST(StatusLineTest, StatusLine_SaysWhatWouldHappenAndWhereItWould)
{
    EditorState state = opened();
    state.moveTo(Point{
        .x = state.view().pan.x + 17, .y = state.view().pan.y + 9});

    const std::string line = statusLine(state, kTranslator);

    EXPECT_NE(line.find("PAINT"), std::string::npos);
    EXPECT_NE(line.find("px 17,9"), std::string::npos);

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
        {.id = MessageId::SaveFailed,
         .detail = "nowhere to write"});

    const std::string line = statusLine(state, kTranslator);

    EXPECT_NE(line.find("UNSAVED"), std::string::npos);
    EXPECT_NE(line.find("save failed"), std::string::npos);
}

TEST(StatusLineTest, StatusLine_IsWordedByWhicheverTranslatorItIsGiven)
{
    constexpr antwika::atlas_editor::Translator swedish{
        antwika::i18n::Locale::Swedish};

    EditorState state = opened();
    state.setStatus(
        {.id = MessageId::SaveFailed,
         .detail = "nowhere to write"});

    const std::string line = statusLine(state, swedish);

    EXPECT_NE(line.find("ÅLA"), std::string::npos);
    EXPECT_NE(line.find("kunde inte spara"), std::string::npos);
    EXPECT_NE(line.find("nowhere to write"), std::string::npos);
    EXPECT_EQ(line.find("PAINT"), std::string::npos);
}

TEST(EditorUiTest, DescribeEditor_LaysTheBarOutFromTheWordsItIsGiven)
{
    constexpr antwika::atlas_editor::Translator swedish{
        antwika::i18n::Locale::Swedish};

    const EditorState state = opened();

    namespace widgets = antwika::atlas_editor::widgets;
    const auto english =
        describeEditor(state, Pointer{}, kTranslator)
            .rects.find(widgets::kFileMenu);
    const auto other = describeEditor(state, Pointer{}, swedish)
                           .rects.find(widgets::kFileMenu);

    ASSERT_TRUE(english.has_value());
    ASSERT_TRUE(other.has_value());
    EXPECT_NE(english->size.width, other->size.width);
}

TEST(EditorUiTest, DescribeEditor_ShowsTheFilesTheModalWasGiven)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showModal(
        antwika::atlas_editor::Modal::Load,
        ".",
        {FileEntry{.name = "one.png"}, FileEntry{.name = "two.png"}});

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_TRUE(
        frame.rects.find(widgets::kFileField).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::kFileConfirm).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::kFileClose).has_value());
    EXPECT_TRUE(
        frame.rects.find(widgets::fileEntryWidget(1)).has_value());
    EXPECT_FALSE(
        frame.rects.find(widgets::fileEntryWidget(2)).has_value());
}

TEST(EditorUiTest, DescribeEditor_HidesTheToolRailBehindTheFileModal)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showModal(antwika::atlas_editor::Modal::Save, ".", {});

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_FALSE(frame.rects.find(widgets::kToolRail).has_value());
}

TEST(EditorUiTest, DescribeEditor_ShowsNoMoreFilesThanTheCardHolds)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showModal(
        antwika::atlas_editor::Modal::Load, ".", manyFiles(20));

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_TRUE(
        frame.rects.find(widgets::fileEntryWidget(kFileRows - 1))
            .has_value());
    EXPECT_FALSE(
        frame.rects.find(widgets::fileEntryWidget(kFileRows))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesNoToolThroughAnOpenMenu)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showMenu(antwika::atlas_editor::Menu::File);

    const auto shown = describeEditor(state, Pointer{}, kTranslator);
    const auto item = shown.rects.find(
        widgets::fileItemWidget(antwika::atlas_editor::FileItem::Load));
    ASSERT_TRUE(item.has_value());

    const auto where = middleOf(*item);
    const auto covered = toolUnder(shown, where);
    ASSERT_TRUE(covered.has_value());

    const auto frame =
        describeEditor(state, Pointer{.position = where}, kTranslator);

    EXPECT_FALSE(
        tipFor(frame, kTranslator.text(toolNameId(*covered)))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_StillNamesAToolBesideAnOpenMenu)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showMenu(antwika::atlas_editor::Menu::File);

    const auto shown = describeEditor(state, Pointer{}, kTranslator);
    const auto button =
        shown.rects.find(widgets::toolWidget(Tool::Ellipse));
    ASSERT_TRUE(button.has_value());

    const auto where = middleOf(*button);
    ASSERT_FALSE(antwika::ui::overlaid(
        shown.overlays, antwika::ui::HoverPointer{.position = where}));

    const auto frame =
        describeEditor(state, Pointer{.position = where}, kTranslator);

    EXPECT_TRUE(
        tipFor(frame, kTranslator.text(MessageId::ToolEllipse))
            .has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesEveryFileTheCardLists)
{
    EXPECT_TRUE(namesEveryFile(kCanvas));
    EXPECT_TRUE(namesEveryFile(kWindow));
    EXPECT_TRUE(namesEveryFile(kWideWindow));
}

TEST(EditorUiTest, DescribeEditor_SpreadsTheInkPickerAcrossTheCanvas)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.toggleInk();

    const auto frame = describeEditor(state, Pointer{}, kTranslator);
    const auto picker = frame.rects.find(widgets::kInkPanel);

    ASSERT_TRUE(picker.has_value());
    EXPECT_EQ(picker->size.width, kCanvas.width);
}

TEST(EditorUiTest, DescribeEditor_ShowsWhatTheSaveCardWillRecord)
{
    EditorState state = opened();
    state.showModal(antwika::atlas_editor::Modal::Save, ".", {});

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    for (const auto &line : metaLines(state.meta(), kTranslator))
    {
        EXPECT_TRUE(drew(frame, line)) << line;
    }
}

TEST(EditorUiTest, DescribeEditor_KeepsTheMetadataOffTheLoadCard)
{
    EditorState state = opened();
    state.showModal(antwika::atlas_editor::Modal::Load, ".", {});

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_FALSE(
        drew(frame, metaLines(state.meta(), kTranslator).front()));
}

TEST(EditorUiTest, DescribeEditor_GivesEveryAtlasFieldSomewhereToClick)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showNewAtlas();

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    for (std::size_t field = 0; field < kAtlasFieldCount; ++field)
    {
        EXPECT_TRUE(
            frame.rects.find(widgets::atlasFieldWidget(field))
                .has_value())
            << field;
    }

    EXPECT_TRUE(frame.rects.find(widgets::kAtlasKind).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kAtlasCreate).has_value());
    EXPECT_TRUE(frame.rects.find(widgets::kFileClose).has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesEveryFieldTheAtlasCardAsksFor)
{
    for (const Size canvas : {kCanvas, kWindow, kWideWindow})
    {
        EditorState state{
            Canvas::blank(kSheet),
            TileGrid{.width = 64, .height = 96},
            canvas};

        state.showNewAtlas();

        const auto frame = describeEditor(state, Pointer{}, kTranslator);

        for (std::size_t field = 0; field < kAtlasFieldCount; ++field)
        {
            const auto named = kTranslator.text(
                fieldNameId(static_cast<AtlasField>(field)));

            EXPECT_TRUE(drew(frame, named)) << named;
        }
    }
}

TEST(EditorUiTest, DescribeEditor_HidesTheToolRailBehindTheAtlasCard)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showNewAtlas();

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_FALSE(frame.rects.find(widgets::kToolRail).has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesTheKindTheAtlasFormCarries)
{
    EditorState state = opened();
    state.showNewAtlas();

    ASSERT_EQ(state.form().kind, antwika::atlas_editor::AtlasKind::Flat);

    EXPECT_TRUE(drew(
        describeEditor(state, Pointer{}, kTranslator),
        kTranslator.text(MessageId::KindFlat)));

    state.turnKind();

    EXPECT_TRUE(drew(
        describeEditor(state, Pointer{}, kTranslator),
        kTranslator.text(MessageId::KindIsometric)));
}

TEST(EditorUiTest, DescribeEditor_OffersEveryPresetOnTheAtlasCard)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showNewAtlas();
    state.showMenu(antwika::atlas_editor::Menu::Preset);

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    for (std::size_t preset = 0; preset < kAtlasPresetCount; ++preset)
    {
        EXPECT_TRUE(
            frame.rects.find(widgets::atlasPresetWidget(preset))
                .has_value())
            << preset;
        EXPECT_TRUE(
            drew(frame, kTranslator.text(presetNameId(preset))))
            << preset;
    }
}

TEST(EditorUiTest, DescribeEditor_KeepsThePresetListShutUntilItIsOpened)
{
    namespace widgets = antwika::atlas_editor::widgets;

    EditorState state = opened();
    state.showNewAtlas();

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_TRUE(frame.rects.find(widgets::kAtlasPresets).has_value());
    EXPECT_FALSE(
        frame.rects.find(widgets::atlasPresetWidget(0)).has_value());
}

TEST(EditorUiTest, DescribeEditor_NamesEveryItemTheViewMenuHolds)
{
    EditorState state = opened();
    state.showMenu(antwika::atlas_editor::Menu::View);

    const auto frame = describeEditor(state, Pointer{}, kTranslator);

    EXPECT_TRUE(drew(frame, kTranslator.text(MessageId::Guides)));
    EXPECT_TRUE(drew(frame, kTranslator.text(MessageId::Pivot)));
    EXPECT_TRUE(drew(frame, kTranslator.text(MessageId::PixelGrid)));
    EXPECT_TRUE(
        drew(frame, kTranslator.text(MessageId::PointerBorder)));
}
