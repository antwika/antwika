#include "antwika/music_editor/SnapshotStore.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "EditorRig.hpp"

using antwika::console::SnapshotError;
using antwika::music_editor::MusicSnapshotStore;
using antwika::music_editor::tests::EditorRig;
using antwika::testing::ScratchDirectory;

namespace
{
    [[nodiscard]] std::string dumpPathIn(const ScratchDirectory &dir)
    {
        return (dir.path() / "dump_state.json").string();
    }

    // A session a few ticks in, so the clocks have something to say.
    void workOn(EditorRig &rig)
    {
        rig.state.source = "$: drum.n(\"0*4\")\n";
        rig.state.scores = {"anthem", "lullaby"};
        rig.score.read(rig.state.source);

        for (int at = 0; at < 3; ++at)
        {
            rig.playback.step(false);
        }
    }
} // namespace

TEST(SnapshotStoreTest, ALoadComesBackToTheDumpedSessionExactly)
{
    const ScratchDirectory dir("antwika_music_snapshot_round.");
    const auto path = dumpPathIn(dir);

    EditorRig rig;
    workOn(rig);

    MusicSnapshotStore store(rig.state, rig.score, rig.playback);
    store.dump(path, {"> dump_state", "dumped state to " + path});

    EditorRig fresh;
    MusicSnapshotStore loading(
        fresh.state, fresh.score, fresh.playback);

    const auto console = loading.load(path);

    EXPECT_EQ(
        console,
        (std::vector<std::string>{
            "> dump_state", "dumped state to " + path}));

    EXPECT_EQ(fresh.state, rig.state);
    EXPECT_EQ(fresh.playback.remember(), rig.playback.remember());
}

// The write is the one thing in a dump that can fail.
TEST(SnapshotStoreTest, ADumpNowhereWritableRefuses)
{
    const ScratchDirectory dir("antwika_music_snapshot_refused.");
    const auto path =
        (dir.path() / "missing" / "dump_state.json").string();

    EditorRig rig;
    MusicSnapshotStore store(rig.state, rig.score, rig.playback);

    EXPECT_THROW(store.dump(path, {"> dump_state"}), SnapshotError);
}

// A table whose arithmetic will not fit is the pattern's refusal.
// The seam only rewraps this application's own error category.
TEST(SnapshotStoreTest, AnOverflowingDumpEscapesAsThePatternsError)
{
    const ScratchDirectory dir("antwika_music_snapshot_overflow.");
    const auto path = dumpPathIn(dir);

    EditorRig rig;
    workOn(rig);

    MusicSnapshotStore store(rig.state, rig.score, rig.playback);
    store.dump(path, {});

    // A second segment so far out that placing it cannot fit.
    nlohmann::json document;
    {
        std::ifstream in(path);
        in >> document;
    }

    nlohmann::json segment;
    segment["startCycle"]["num"] =
        std::numeric_limits<std::int64_t>::max();
    segment["startCycle"]["den"] = 1;
    segment["framesPerCycle"]["num"] = 48000;
    segment["framesPerCycle"]["den"] = 1;
    document["state"]["playback"]["segments"].push_back(segment);

    {
        std::ofstream out(path);
        out << document.dump();
    }

    EditorRig fresh;
    MusicSnapshotStore loading(
        fresh.state, fresh.score, fresh.playback);

    EXPECT_THROW(
        (void)loading.load(path), antwika::pattern::PatternError);
}

// The codec's refusal is this application's own error category.
// What the console seam promises is console::SnapshotError.
TEST(SnapshotStoreTest, ADumpWhoseStateCannotStandRefusesAsSnapshot)
{
    const ScratchDirectory dir("antwika_music_snapshot_broken.");
    const auto path = dumpPathIn(dir);

    EditorRig rig;
    workOn(rig);

    MusicSnapshotStore store(rig.state, rig.score, rig.playback);
    store.dump(path, {});

    // A first segment away from zero is a table with a hole.
    nlohmann::json document;
    {
        std::ifstream in(path);
        in >> document;
    }

    document["state"]["playback"]["segments"][0]["startCycle"]
            ["num"] = 1;

    {
        std::ofstream out(path);
        out << document.dump();
    }

    EditorRig fresh;
    MusicSnapshotStore loading(
        fresh.state, fresh.score, fresh.playback);

    EXPECT_THROW((void)loading.load(path), SnapshotError);

    // The refusal landed before anything was mutated.
    EXPECT_EQ(fresh.state, EditorRig{}.state);
}
