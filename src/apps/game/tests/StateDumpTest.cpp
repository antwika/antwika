#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

#include <antwika/console/SnapshotFormat.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/SchemaVersion.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/SaveFormatError.hpp"
#include "antwika/game/StateDump.hpp"

using antwika::game::BuildTool;
using antwika::game::Camera;
using antwika::game::MapView;
using antwika::game::SaveFormatError;
using antwika::game::StateDump;
using antwika::game::stateDumpFromJson;
using antwika::game::stateDumpToJson;

namespace
{
    [[nodiscard]] StateDump populatedDump()
    {
        StateDump dump;

        dump.save.state = {
            .ticksProcessed = 12, .score = 3, .money = 4400};
        dump.save.paths = {{.x = 2, .y = 3}, {.x = 2, .y = 4}};
        dump.save.camera = Camera(antwika::gfx::Point{.x = 40, .y = 8}, 1);
        dump.save.seed = 7;
        dump.paused = true;
        dump.tool = BuildTool::Well;
        dump.view = MapView::Water;
        dump.locale = antwika::i18n::Locale::Swedish;

        return dump;
    }

    [[nodiscard]] antwika::console::SnapshotFormat gameFormat()
    {
        return antwika::console::SnapshotFormat(
            {.magic = antwika::game::kStateDumpMagic,
             .version = antwika::game::kStateDumpVersion},
            "antwika game state dump document",
            antwika::game::standardStateDumpMigrations);
    }
} // namespace

TEST(StateDumpTest, RoundTrip_EveryFieldSurvives)
{
    const auto dump = populatedDump();

    EXPECT_EQ(stateDumpFromJson(stateDumpToJson(dump)), dump);
}

TEST(StateDumpTest, RoundTrip_APaletteThatWasDownStaysDown)
{
    auto dump = populatedDump();
    dump.tool = std::nullopt;

    const auto encoded = stateDumpToJson(dump);

    // Absent rather than a name for no tool.
    EXPECT_FALSE(encoded.contains("tool"));
    EXPECT_EQ(stateDumpFromJson(encoded), dump);
}

TEST(StateDumpTest, EqualityComparesEveryField)
{
    const auto base = populatedDump();

    auto city = populatedDump();
    city.save.paths.clear();
    EXPECT_NE(base, city);

    auto held = populatedDump();
    held.paused = false;
    EXPECT_NE(base, held);

    auto palette = populatedDump();
    palette.tool = std::nullopt;
    EXPECT_NE(base, palette);

    auto viewed = populatedDump();
    viewed.view = MapView::Normal;
    EXPECT_NE(base, viewed);

    auto worded = populatedDump();
    worded.locale = antwika::i18n::kDefaultLocale;
    EXPECT_NE(base, worded);
}

TEST(StateDumpTest, FromJson_RefusesAMissingMember)
{
    auto state = stateDumpToJson(populatedDump());
    state.erase("paused");

    EXPECT_THROW((void)stateDumpFromJson(state), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesAToolThisBuildDoesNotKnow)
{
    auto state = stateDumpToJson(populatedDump());
    state["tool"] = "bulldozer";

    EXPECT_THROW((void)stateDumpFromJson(state), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesAMapViewThisBuildDoesNotKnow)
{
    auto state = stateDumpToJson(populatedDump());
    state["mapView"] = "crime";

    EXPECT_THROW((void)stateDumpFromJson(state), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesALanguageWithNoCatalogue)
{
    auto state = stateDumpToJson(populatedDump());
    state["locale"] = "fr";

    EXPECT_THROW((void)stateDumpFromJson(state), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesASaveTheSaveFormatRefuses)
{
    auto state = stateDumpToJson(populatedDump());
    state["save"]["magic"] = "not-a-save";

    // The embedded save polices itself, through its own reader.
    EXPECT_THROW((void)stateDumpFromJson(state), SaveFormatError);
}

// Version 2 moved the state under the shared envelope.
// A version 1 file was this application's own bespoke shape.
// It still loads, members meaning exactly what they meant.
TEST(StateDumpTest, AVersionOneDocumentIsReadThroughTheEnvelope)
{
    const auto dump = populatedDump();

    nlohmann::json old = stateDumpToJson(dump);
    old["magic"] = std::string(antwika::game::kStateDumpMagic);
    old[std::string(antwika::replay::kSchemaVersionKey)] = 1U;
    old["console"] = {"> dump_state"};

    const auto snapshot = gameFormat().fromJson(old);

    EXPECT_EQ(
        snapshot.console,
        (std::vector<std::string>{"> dump_state"}));
    EXPECT_EQ(stateDumpFromJson(snapshot.state), dump);
}

TEST(StateDumpTest, AVersionOneDocumentMayLackAMember)
{
    // A v1 document missing a member migrates without inventing it.
    // The decoder is what refuses the migrated state, not the chain.
    nlohmann::json old = stateDumpToJson(populatedDump());
    old.erase("tool");
    old["magic"] = std::string(antwika::game::kStateDumpMagic);
    old[std::string(antwika::replay::kSchemaVersionKey)] = 1U;
    old["console"] = nlohmann::json::array();

    const auto snapshot = gameFormat().fromJson(old);

    EXPECT_FALSE(snapshot.state.contains("tool"));
    EXPECT_THROW(
        (void)stateDumpFromJson(snapshot.state), SaveFormatError);
}
