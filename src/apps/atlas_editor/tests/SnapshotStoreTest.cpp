#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/gfx/PngWriter.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorSnapshotStore.hpp"
#include "antwika/atlas_editor/TileGrid.hpp"
#include "antwika/atlas_editor/Tool.hpp"

using antwika::atlas_editor::Canvas;
using antwika::atlas_editor::EditorSnapshotStore;
using antwika::atlas_editor::EditorState;
using antwika::atlas_editor::TileGrid;
using antwika::atlas_editor::Tool;
using antwika::console::SnapshotError;
using antwika::gfx::Size;
using antwika::testing::ScratchDirectory;
using ::testing::HasSubstr;

namespace
{
    constexpr Size kCanvas{.width = 800, .height = 480};
    constexpr Size kSheet{.width = 8, .height = 8};

    [[nodiscard]] EditorState opened()
    {
        return EditorState{
            Canvas::blank(kSheet), TileGrid{}, kCanvas};
    }

    void workOn(EditorState &state)
    {
        state.selectTool(Tool::Paint);
        state.selectColor(2);
        state.toggleGrid();
        state.toggleGuides();
        state.applyAt({.x = 400, .y = 240});
        state.selectTool(Tool::Select);
        state.beginSelecting({.x = 0, .y = 0});
        state.finishSelecting({.x = 799, .y = 479});
        state.copySelection();
        state.zoomIn({.x = 0, .y = 0});
        state.panBy({.x = 3, .y = -2});
        state.noteTick();
    }

    [[nodiscard]] std::string dumpPathIn(const ScratchDirectory &dir)
    {
        return (dir.path() / "dump_state.json").string();
    }
}

TEST(SnapshotStoreTest, Dump_WritesTheDocumentAndItsSheetPng)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_files.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    EditorSnapshotStore store(state);

    store.dump(path, {"> dump_state"});

    EXPECT_TRUE(std::filesystem::exists(path));
    EXPECT_TRUE(std::filesystem::exists(
        dir.path() / "dump_state.sheet.png"));

    EXPECT_FALSE(std::filesystem::exists(
        dir.path() / "dump_state.clipboard.png"));
}

TEST(SnapshotStoreTest, Dump_GivesAHeldClipboardItsOwnSidePng)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_clip.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    workOn(state);
    ASSERT_TRUE(state.hasClipboard());

    EditorSnapshotStore store(state);
    store.dump(path, {});

    EXPECT_TRUE(std::filesystem::exists(
        dir.path() / "dump_state.clipboard.png"));
}

TEST(SnapshotStoreTest, Load_ComesBackToTheDumpedStateExactly)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_round.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    workOn(state);

    EditorSnapshotStore store(state);
    store.dump(path, {"> dump_state", "dumped state to " + path});

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    const auto console = loading.load(path);

    EXPECT_EQ(
        console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));

    EXPECT_EQ(fresh.image().bitmap(), state.image().bitmap());
    EXPECT_EQ(fresh.image().revision(), state.image().revision());
    EXPECT_EQ(fresh.view(), state.view());
    EXPECT_EQ(fresh.tool(), state.tool());
    EXPECT_EQ(fresh.color(), state.color());
    EXPECT_EQ(fresh.colorIndex(), state.colorIndex());
    EXPECT_EQ(fresh.gridVisible(), state.gridVisible());
    EXPECT_EQ(fresh.guidesVisible(), state.guidesVisible());
    EXPECT_EQ(fresh.hovered(), state.hovered());
    EXPECT_EQ(fresh.selection(), state.selection());
    EXPECT_EQ(fresh.currentGesture(), state.currentGesture());
    ASSERT_TRUE(fresh.hasClipboard());
    EXPECT_EQ(
        fresh.clipboardImage()->bitmap(),
        state.clipboardImage()->bitmap());
    EXPECT_EQ(fresh.edits(), state.edits());
    EXPECT_EQ(fresh.ticks(), state.ticks());
    EXPECT_EQ(fresh.saves(), state.saves());
    EXPECT_EQ(fresh.loads(), state.loads());
    EXPECT_EQ(fresh.savedAtRevision(), state.savedAtRevision());
    EXPECT_EQ(fresh.unsaved(), state.unsaved());

    EXPECT_FALSE(fresh.status().has_value());
}

TEST(SnapshotStoreTest, Load_KeepsAGestureMidDrag)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_drag.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    state.selectTool(Tool::Select);
    state.beginSelecting({.x = 10, .y = 10});
    state.dragSelectionTo({.x = 40, .y = 40});
    ASSERT_TRUE(state.currentGesture().has_value());

    EditorSnapshotStore store(state);
    store.dump(path, {});

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);
    (void)loading.load(path);

    EXPECT_EQ(fresh.currentGesture(), state.currentGesture());
    EXPECT_EQ(fresh.shownSelection(), state.shownSelection());
}

TEST(SnapshotStoreTest, Load_RefusesATamperedSheetPng)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_tamper.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    EditorSnapshotStore store(state);
    store.dump(path, {});

    {
        auto other = Canvas::blank(kSheet);
        (void)other.set({.x = 0, .y = 0}, {.red = 9});
        std::ofstream file(
            dir.path() / "dump_state.sheet.png",
            std::ios::binary | std::ios::trunc);
        antwika::gfx::PngWriter{}.write(other.bitmap(), file);
    }

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    EXPECT_THROW((void)loading.load(path), SnapshotError);
}

TEST(SnapshotStoreTest, Load_RefusesAMissingSheetPng)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_gone.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    EditorSnapshotStore store(state);
    store.dump(path, {});

    std::filesystem::remove(dir.path() / "dump_state.sheet.png");

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    EXPECT_THROW((void)loading.load(path), SnapshotError);
}

TEST(SnapshotStoreTest, Load_RefusesAMissingClipboardPng)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_clipgone.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    workOn(state);
    EditorSnapshotStore store(state);
    store.dump(path, {});

    std::filesystem::remove(dir.path() / "dump_state.clipboard.png");

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    EXPECT_THROW((void)loading.load(path), SnapshotError);
}

TEST(SnapshotStoreTest, Load_RefusesABadFieldAsSnapshotError)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_badfield.");
    const auto path = dumpPathIn(dir);

    EditorState state = opened();
    EditorSnapshotStore store(state);
    store.dump(path, {});

    {
        std::ifstream in(path);
        auto document = nlohmann::json::parse(in);
        in.close();
        document["state"]["tool"] = "chainsaw";
        std::ofstream out(path, std::ios::trunc);
        out << document.dump();
    }

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    try
    {
        (void)loading.load(path);
        FAIL() << "a bad tool name must refuse";
    }
    catch (const SnapshotError &refused)
    {
        EXPECT_THAT(refused.what(), HasSubstr("chainsaw"));
    }
}

TEST(SnapshotStoreTest, Load_RefusesAnotherApplicationsDump)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_magic.");
    const auto path = dumpPathIn(dir);

    {
        std::ofstream out(path);
        out << R"({"magic":"antwika-game-state-dump","version":1,)"
            << R"("console":[],"state":{}})";
    }

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    EXPECT_THROW((void)loading.load(path), SnapshotError);
}

TEST(SnapshotStoreTest, Dump_RefusesAnUnwritablePath)
{
    EditorState state = opened();
    EditorSnapshotStore store(state);

    EXPECT_THROW(
        store.dump("/nonexistent/nowhere/dump_state.json", {}),
        SnapshotError);
}

TEST(SnapshotStoreTest, Dump_WritesSidePngsWithoutAJsonSuffix)
{
    const ScratchDirectory dir("antwika_atlas_snapshot_suffix.");
    const auto path = (dir.path() / "dump_state").string();

    EditorState state = opened();
    EditorSnapshotStore store(state);
    store.dump(path, {});

    EXPECT_TRUE(std::filesystem::exists(
        dir.path() / "dump_state.sheet.png"));

    EditorState fresh = opened();
    EditorSnapshotStore loading(fresh);

    EXPECT_EQ(loading.load(path), std::vector<std::string>{});
}
