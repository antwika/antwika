#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <antwika/console/SnapshotError.hpp>
#include <antwika/pattern/PatternError.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/music_editor/MusicSnapshotStore.hpp"
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
}

TEST(SnapshotStoreTest, Load_ComesBackToTheDumpedSession)
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

TEST(SnapshotStoreTest, Dump_ADumpNowhereWritableRefuses)
{
    const ScratchDirectory dir("antwika_music_snapshot_refused.");
    const auto path =
        (dir.path() / "missing" / "dump_state.json").string();

    EditorRig rig;
    MusicSnapshotStore store(rig.state, rig.score, rig.playback);

    EXPECT_THROW(store.dump(path, {"> dump_state"}), SnapshotError);
}

TEST(SnapshotStoreTest, Dump_EscapesAnOverflowAsPatternError)
{
    const ScratchDirectory dir("antwika_music_snapshot_overflow.");
    const auto path = dumpPathIn(dir);

    EditorRig rig;
    workOn(rig);

    MusicSnapshotStore store(rig.state, rig.score, rig.playback);
    store.dump(path, {});

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

TEST(SnapshotStoreTest, Dump_RefusesAnImpossibleStateAsSnapshot)
{
    const ScratchDirectory dir("antwika_music_snapshot_broken.");
    const auto path = dumpPathIn(dir);

    EditorRig rig;
    workOn(rig);

    MusicSnapshotStore store(rig.state, rig.score, rig.playback);
    store.dump(path, {});

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

    EXPECT_EQ(fresh.state, EditorRig{}.state);
}
