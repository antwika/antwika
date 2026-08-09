#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <optional>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/CanvasView.hpp"
#include "antwika/atlas_editor/Pixel.hpp"
#include "antwika/atlas_editor/Selection.hpp"
#include "antwika/atlas_editor/StateDump.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::AtlasEditorError;
using antwika::atlas_editor::DumpedImage;
using antwika::atlas_editor::EditorStateDump;
using antwika::atlas_editor::Gesture;
using antwika::atlas_editor::kToolCount;
using antwika::atlas_editor::kZoomScales;
using antwika::atlas_editor::Pixel;
using antwika::atlas_editor::Selection;
using antwika::atlas_editor::stateDumpFromJson;
using antwika::atlas_editor::stateDumpToJson;
using antwika::atlas_editor::Tool;

namespace
{
    [[nodiscard]] EditorStateDump fullDump()
    {
        EditorStateDump dump;

        dump.sheet = DumpedImage{
            .size = {.width = 8, .height = 12}, .fingerprint = 77};
        dump.sheetRevision = 5;
        dump.clipboard = DumpedImage{
            .size = {.width = 2, .height = 3}, .fingerprint = 91};
        dump.view = {.pan = {.x = -4, .y = 9}, .zoom = 3};
        dump.tool = Tool::Fill;
        dump.paint = {.red = 1, .green = 2, .blue = 3, .alpha = 4};
        dump.swatch = 6;
        dump.showGrid = false;
        dump.showGuides = false;
        dump.under = Pixel{.x = -1, .y = 2};
        dump.marked = Selection{
            .origin = {.x = 1, .y = 2},
            .size = {.width = 3, .height = 4}};
        dump.gesture = Gesture{
            .carrying = true,
            .from = {.x = 0, .y = 1},
            .to = {.x = 5, .y = 6}};
        dump.preview = antwika::atlas_editor::PreviewPane{
            .open = true,
            .autoFocus = false,
            .ratio = 250,
            .dragging = false,
            .view = {.pan = {.x = 7, .y = -3}, .zoom = 4},
            .focused = 11};
        dump.changes = 10;
        dump.stepped = 11;
        dump.written = 12;
        dump.read = 13;
        dump.savedRevision = 14;

        return dump;
    }

    [[nodiscard]] EditorStateDump bareDump()
    {
        EditorStateDump dump;

        dump.sheet = DumpedImage{
            .size = {.width = 4, .height = 4}, .fingerprint = 1};

        return dump;
    }
}

TEST(StateDumpTest, StateDumpFromJson_KeepsEveryPresentOptional)
{
    const auto dump = fullDump();

    EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
}

TEST(StateDumpTest, StateDumpFromJson_KeepsEveryAbsentOptional)
{
    const auto dump = bareDump();

    const auto encoded = stateDumpToJson(dump);

    EXPECT_FALSE(encoded.contains("clipboard"));
    EXPECT_FALSE(encoded.contains("swatch"));
    EXPECT_FALSE(encoded.contains("under"));
    EXPECT_FALSE(encoded.contains("marked"));
    EXPECT_FALSE(encoded.contains("gesture"));

    EXPECT_EQ(stateDumpFromJson(encoded), dump);
}

TEST(StateDumpTest, StateDumpFromJson_KeepsEveryToolName)
{
    for (std::size_t index = 0; index < kToolCount; ++index)
    {
        auto dump = bareDump();
        dump.tool = static_cast<Tool>(index);

        EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
    }
}

TEST(StateDumpTest, StateDumpFromJson_KeepsEveryZoomLevel)
{
    for (std::size_t zoom = 0; zoom < kZoomScales.size(); ++zoom)
    {
        auto dump = bareDump();
        dump.view.zoom = zoom;

        EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
    }
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnknownToolName)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["tool"] = "chainsaw";

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAZoomPastTheTable)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["view"]["zoom"] = kZoomScales.size();

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAMissingMember)
{
    for (const auto *member :
         {"sheet", "view", "tool", "paint", "showGrid", "showGuides",
          "counters"})
    {
        auto encoded = stateDumpToJson(fullDump());
        encoded.erase(member);

        EXPECT_THROW(
            (void)stateDumpFromJson(encoded), AtlasEditorError)
            << member;
    }
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAnUnnamedMember)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["undoStack"] = nlohmann::json::array();

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAWrongTypeBySchema)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["showGrid"] = "yes";

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryDumpedImageField)
{
    const DumpedImage base{
        .size = {.width = 8, .height = 12}, .fingerprint = 77};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto resized = base;
    resized.size = {.width = 9, .height = 12};
    EXPECT_NE(base, resized);

    auto reprinted = base;
    reprinted.fingerprint = 78;
    EXPECT_NE(base, reprinted);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryGestureField)
{
    const Gesture base{
        .carrying = true, .from = {.x = 0, .y = 1}, .to = {.x = 5, .y = 6}};

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto drawing = base;
    drawing.carrying = false;
    EXPECT_NE(base, drawing);

    auto moved = base;
    moved.from = Pixel{.x = 2, .y = 1};
    EXPECT_NE(base, moved);

    auto reached = base;
    reached.to = Pixel{.x = 5, .y = 7};
    EXPECT_NE(base, reached);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryField)
{
    const auto base = fullDump();

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto resheeted = base;
    resheeted.sheet.fingerprint = 1;
    EXPECT_NE(base, resheeted);

    auto revised = base;
    revised.sheetRevision = 99;
    EXPECT_NE(base, revised);

    auto emptied = base;
    emptied.clipboard.reset();
    EXPECT_NE(base, emptied);

    auto panned = base;
    panned.view.pan.x = 100;
    EXPECT_NE(base, panned);

    auto retooled = base;
    retooled.tool = Tool::Paint;
    EXPECT_NE(base, retooled);

    auto repainted = base;
    repainted.paint.red = 200;
    EXPECT_NE(base, repainted);

    auto reswatched = base;
    reswatched.swatch.reset();
    EXPECT_NE(base, reswatched);

    auto gridded = base;
    gridded.showGrid = true;
    EXPECT_NE(base, gridded);

    auto guided = base;
    guided.showGuides = true;
    EXPECT_NE(base, guided);

    auto pivoted = base;
    pivoted.showPivot = !base.showPivot;
    EXPECT_NE(base, pivoted);

    auto ringed = base;
    ringed.showPointerBorder = !base.showPointerBorder;
    EXPECT_NE(base, ringed);

    auto hovered = base;
    hovered.under = Pixel{.x = 7, .y = 7};
    EXPECT_NE(base, hovered);

    auto unmarked = base;
    unmarked.marked.reset();
    EXPECT_NE(base, unmarked);

    auto released = base;
    released.gesture.reset();
    EXPECT_NE(base, released);

    auto changed = base;
    changed.changes = 999;
    EXPECT_NE(base, changed);

    auto stepped = base;
    stepped.stepped = 999;
    EXPECT_NE(base, stepped);

    auto written = base;
    written.written = 999;
    EXPECT_NE(base, written);

    auto read = base;
    read.read = 999;
    EXPECT_NE(base, read);

    auto saved = base;
    saved.savedRevision = 999;
    EXPECT_NE(base, saved);
}

TEST(StateDumpTest, StateDumpFromJson_ReadsBackTheViewTogglesItWrote)
{
    auto dump = fullDump();
    dump.showPivot = true;
    dump.showPointerBorder = false;

    const auto read = stateDumpFromJson(stateDumpToJson(dump));

    EXPECT_TRUE(read.showPivot);
    EXPECT_FALSE(read.showPointerBorder);
}

TEST(StateDumpTest, StateDumpFromJson_FallsBackForADumpFromBefore)
{
    auto encoded = stateDumpToJson(fullDump());
    encoded.erase("showPivot");
    encoded.erase("showPointerBorder");

    const auto read = stateDumpFromJson(encoded);

    EXPECT_FALSE(read.showPivot);
    EXPECT_TRUE(read.showPointerBorder);
}

TEST(StateDumpTest, StateDumpFromJson_TakesADumpThatNamesNoPreview)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded.erase("preview");

    EXPECT_EQ(
        stateDumpFromJson(encoded).preview,
        antwika::atlas_editor::PreviewPane{});
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAPreviewZoomPastTheTable)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["preview"]["zoom"] =
        kZoomScales.size();

    EXPECT_THROW(
        (void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, OperatorEquals_ComparesThePreviewPane)
{
    const auto dump = bareDump();

    auto other = dump;
    other.preview.open = !dump.preview.open;

    EXPECT_NE(dump, other);
}
