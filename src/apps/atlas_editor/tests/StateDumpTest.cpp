#include <cstddef>
#include <optional>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

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
    // Every optional present, every counter distinct.
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
        dump.changes = 10;
        dump.stepped = 11;
        dump.written = 12;
        dump.read = 13;
        dump.savedRevision = 14;

        return dump;
    }

    // Every optional absent.
    [[nodiscard]] EditorStateDump bareDump()
    {
        EditorStateDump dump;

        dump.sheet = DumpedImage{
            .size = {.width = 4, .height = 4}, .fingerprint = 1};

        return dump;
    }
} // namespace

TEST(StateDumpTest, EveryOptionalPresentSurvivesTheRoundTrip)
{
    const auto dump = fullDump();

    EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
}

TEST(StateDumpTest, EveryOptionalAbsentSurvivesTheRoundTrip)
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

TEST(StateDumpTest, EveryToolNameSurvivesTheRoundTrip)
{
    for (std::size_t index = 0; index < kToolCount; ++index)
    {
        auto dump = bareDump();
        dump.tool = static_cast<Tool>(index);

        EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
    }
}

TEST(StateDumpTest, EveryZoomLevelTheTableHoldsSurvives)
{
    for (std::size_t zoom = 0; zoom < kZoomScales.size(); ++zoom)
    {
        auto dump = bareDump();
        dump.view.zoom = zoom;

        EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
    }
}

TEST(StateDumpTest, AnUnknownToolNameIsRefused)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["tool"] = "chainsaw";

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, AZoomLevelPastTheTableIsRefused)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["view"]["zoom"] = kZoomScales.size();

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, AMissingMemberIsRefused)
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

TEST(StateDumpTest, AMemberTheSchemaDoesNotNameIsRefused)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["undoStack"] = nlohmann::json::array();

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, AWrongTypeIsRefusedBySchemaNotByAccessor)
{
    auto encoded = stateDumpToJson(bareDump());
    encoded["showGrid"] = "yes";

    EXPECT_THROW((void)stateDumpFromJson(encoded), AtlasEditorError);
}

TEST(StateDumpTest, DumpedImageEqualityComparesEveryField)
{
    const DumpedImage base{
        .size = {.width = 8, .height = 12}, .fingerprint = 77};

    EXPECT_EQ(base, base);

    auto resized = base;
    resized.size = {.width = 9, .height = 12};
    EXPECT_NE(base, resized);

    auto reprinted = base;
    reprinted.fingerprint = 78;
    EXPECT_NE(base, reprinted);
}

TEST(StateDumpTest, GestureEqualityComparesEveryField)
{
    const Gesture base{
        .carrying = true, .from = {.x = 0, .y = 1}, .to = {.x = 5, .y = 6}};

    EXPECT_EQ(base, base);

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

TEST(StateDumpTest, EqualityComparesEveryField)
{
    const auto base = fullDump();

    EXPECT_EQ(base, base);

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
