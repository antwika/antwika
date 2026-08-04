#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

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
        dump.console = {"> dump_state", "dumped state to dump_state.json"};

        return dump;
    }

    [[nodiscard]] std::string versionKey()
    {
        return std::string(antwika::replay::kSchemaVersionKey);
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

    auto quiet = populatedDump();
    quiet.console.clear();
    EXPECT_NE(base, quiet);
}

TEST(StateDumpTest, FromJson_RefusesTheWrongMagic)
{
    auto document = stateDumpToJson(populatedDump());
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesADocumentFromANewerBuild)
{
    auto document = stateDumpToJson(populatedDump());
    document[versionKey()] = antwika::game::kStateDumpVersion + 1;

    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesAMissingMember)
{
    auto document = stateDumpToJson(populatedDump());
    document.erase("paused");

    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesAToolThisBuildDoesNotKnow)
{
    auto document = stateDumpToJson(populatedDump());
    document["tool"] = "bulldozer";

    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesAMapViewThisBuildDoesNotKnow)
{
    auto document = stateDumpToJson(populatedDump());
    document["mapView"] = "crime";

    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesALanguageWithNoCatalogue)
{
    auto document = stateDumpToJson(populatedDump());
    document["locale"] = "fr";

    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}

TEST(StateDumpTest, FromJson_RefusesASaveTheSaveFormatRefuses)
{
    auto document = stateDumpToJson(populatedDump());
    document["save"]["magic"] = "not-a-save";

    // The embedded save polices itself, through its own reader.
    EXPECT_THROW((void)stateDumpFromJson(document), SaveFormatError);
}
