#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <filesystem>
#include <string>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/log/mocks/MockLogger.hpp>
#include <antwika/testing/ScratchPath.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/GenerationRules.hpp"

using antwika::log::mocks::MockLogger;
using antwika::map_editor::defaultGenerationRules;
using antwika::map_editor::GenerationRules;
using antwika::map_editor::kTerrainCount;
using antwika::map_editor::loadRulesFileOrDefaults;
using antwika::map_editor::rulesFromJson;
using antwika::map_editor::rulesToJson;
using antwika::map_editor::saveRulesFile;
using antwika::testing::ScratchDirectory;
using antwika::tilemap::TerrainClass;
using ::testing::_;
using ::testing::NiceMock;

namespace
{
    [[nodiscard]] bool allows(
        const GenerationRules &rules,
        const TerrainClass left,
        const TerrainClass right)
    {
        return rules.allowed[antwika::enums::index(left)]
                            [antwika::enums::index(right)];
    }

    [[nodiscard]] double weightOf(
        const GenerationRules &rules, const TerrainClass terrain)
    {
        return rules.weights[antwika::enums::index(terrain)];
    }

    [[nodiscard]] nlohmann::json validDocument()
    {
        return rulesToJson(defaultGenerationRules());
    }
}

TEST(GenerationRulesTest, DefaultGenerationRules_UsesTheShippedWeights)
{
    const auto rules = defaultGenerationRules();

    EXPECT_DOUBLE_EQ(weightOf(rules, TerrainClass::Floor), 8.0);
    EXPECT_DOUBLE_EQ(weightOf(rules, TerrainClass::Wall), 3.0);
    EXPECT_DOUBLE_EQ(weightOf(rules, TerrainClass::Water), 2.0);
    EXPECT_DOUBLE_EQ(weightOf(rules, TerrainClass::Cliff), 1.0);
    EXPECT_DOUBLE_EQ(weightOf(rules, TerrainClass::Path), 2.0);
    EXPECT_DOUBLE_EQ(weightOf(rules, TerrainClass::Stair), 1.0);
}

TEST(GenerationRulesTest, DefaultGenerationRules_KeepTheMatrixSymmetric)
{
    const auto rules = defaultGenerationRules();

    for (std::size_t a = 0; a < kTerrainCount; ++a)
    {
        for (std::size_t b = 0; b < kTerrainCount; ++b)
        {
            EXPECT_EQ(rules.allowed[a][b], rules.allowed[b][a]);
        }
    }
}

TEST(GenerationRulesTest, DefaultGenerationRules_ForbidTheDeclaredPairs)
{
    const auto rules = defaultGenerationRules();

    EXPECT_FALSE(allows(rules, TerrainClass::Wall, TerrainClass::Water));
    EXPECT_FALSE(allows(rules, TerrainClass::Wall, TerrainClass::Cliff));
    EXPECT_FALSE(allows(rules, TerrainClass::Water, TerrainClass::Path));
    EXPECT_FALSE(allows(rules, TerrainClass::Water, TerrainClass::Cliff));
    EXPECT_FALSE(allows(rules, TerrainClass::Water, TerrainClass::Stair));
    EXPECT_FALSE(allows(rules, TerrainClass::Path, TerrainClass::Cliff));
    EXPECT_FALSE(allows(rules, TerrainClass::Cliff, TerrainClass::Stair));
}

TEST(GenerationRulesTest, DefaultGenerationRules_AllowTheOtherPairs)
{
    const auto rules = defaultGenerationRules();

    EXPECT_TRUE(allows(rules, TerrainClass::Floor, TerrainClass::Wall));
    EXPECT_TRUE(allows(rules, TerrainClass::Floor, TerrainClass::Floor));
    EXPECT_TRUE(allows(rules, TerrainClass::Path, TerrainClass::Stair));
}

TEST(GenerationRulesTest, OperatorEquals_ComparesWeightsAndAdjacency)
{
    const auto base = defaultGenerationRules();

    EXPECT_EQ(base, defaultGenerationRules());

    auto reweighted = base;
    reweighted.weights[0] = 9.0;
    EXPECT_NE(base, reweighted);

    auto rewired = base;
    rewired.allowed[0][0] = !rewired.allowed[0][0];
    EXPECT_NE(base, rewired);
}

TEST(GenerationRulesTest, RulesToJson_OmitsTheStairWeight)
{
    const auto document = rulesToJson(defaultGenerationRules());

    EXPECT_TRUE(document.at("weights").contains("floor"));
    EXPECT_FALSE(document.at("weights").contains("stair"));
}

TEST(GenerationRulesTest, RulesToJson_WritesEachAllowedPairOnce)
{
    const auto document = rulesToJson(defaultGenerationRules());

    for (const auto &entry : document.at("adjacency"))
    {
        ASSERT_TRUE(entry.is_array());
        EXPECT_EQ(entry.size(), 2U);
    }

    EXPECT_FALSE(document.at("adjacency").empty());
}

TEST(GenerationRulesTest, RulesFromJson_RoundTripsTheDefaults)
{
    const auto rules = rulesFromJson(validDocument());

    ASSERT_TRUE(rules.has_value());
    EXPECT_EQ(*rules, defaultGenerationRules());
}

TEST(GenerationRulesTest, RulesFromJson_AppliesEachPairSymmetrically)
{
    nlohmann::json document;
    document["weights"] = nlohmann::json::object();
    document["adjacency"] = nlohmann::json::array(
        {nlohmann::json::array({"floor", "wall"})});

    const auto rules = rulesFromJson(document);

    ASSERT_TRUE(rules.has_value());
    EXPECT_TRUE(allows(*rules, TerrainClass::Floor, TerrainClass::Wall));
    EXPECT_TRUE(allows(*rules, TerrainClass::Wall, TerrainClass::Floor));
    EXPECT_FALSE(
        allows(*rules, TerrainClass::Floor, TerrainClass::Water));
}

TEST(GenerationRulesTest, RulesFromJson_KeepsTheStairWeightAtItsDefault)
{
    nlohmann::json document;
    document["weights"] = nlohmann::json::object();
    document["weights"]["floor"] = 5.0;
    document["adjacency"] = nlohmann::json::array();

    const auto rules = rulesFromJson(document);

    ASSERT_TRUE(rules.has_value());
    EXPECT_DOUBLE_EQ(weightOf(*rules, TerrainClass::Floor), 5.0);
    EXPECT_DOUBLE_EQ(weightOf(*rules, TerrainClass::Stair), 1.0);
}

TEST(GenerationRulesTest, RulesFromJson_RefusesADocumentThatIsNotAnObject)
{
    EXPECT_FALSE(rulesFromJson(nlohmann::json(5)).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAMissingSection)
{
    auto without = validDocument();
    without.erase("weights");
    EXPECT_FALSE(rulesFromJson(without).has_value());

    auto unpaired = validDocument();
    unpaired.erase("adjacency");
    EXPECT_FALSE(rulesFromJson(unpaired).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesASectionOfTheWrongKind)
{
    auto document = validDocument();
    document["weights"] = 5;
    EXPECT_FALSE(rulesFromJson(document).has_value());

    document = validDocument();
    document["adjacency"] = 5;
    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAnUnknownTerrainWeight)
{
    auto document = validDocument();
    document["weights"]["lava"] = 2.0;

    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAWeightOnTheStair)
{
    auto document = validDocument();
    document["weights"]["stair"] = 2.0;

    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAWeightThatIsNotANumber)
{
    auto document = validDocument();
    document["weights"]["floor"] = "heavy";

    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAWeightAtOrBelowZero)
{
    auto document = validDocument();
    document["weights"]["floor"] = 0.0;
    EXPECT_FALSE(rulesFromJson(document).has_value());

    document = validDocument();
    document["weights"]["floor"] = -1.0;
    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAPairOfTheWrongShape)
{
    auto document = validDocument();
    document["adjacency"] = nlohmann::json::array({5});
    EXPECT_FALSE(rulesFromJson(document).has_value());

    document = validDocument();
    document["adjacency"] =
        nlohmann::json::array({nlohmann::json::array({"floor"})});
    EXPECT_FALSE(rulesFromJson(document).has_value());

    document = validDocument();
    document["adjacency"] = nlohmann::json::array(
        {nlohmann::json::array({5, "wall"})});
    EXPECT_FALSE(rulesFromJson(document).has_value());

    document = validDocument();
    document["adjacency"] = nlohmann::json::array(
        {nlohmann::json::array({"floor", 5})});
    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, RulesFromJson_RefusesAPairNamingAnUnknownTerrain)
{
    auto document = validDocument();
    document["adjacency"] = nlohmann::json::array(
        {nlohmann::json::array({"lava", "wall"})});
    EXPECT_FALSE(rulesFromJson(document).has_value());

    document = validDocument();
    document["adjacency"] = nlohmann::json::array(
        {nlohmann::json::array({"floor", "lava"})});
    EXPECT_FALSE(rulesFromJson(document).has_value());
}

TEST(GenerationRulesTest, LoadRulesFileOrDefaults_ReadsAWrittenFile)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("rules.");
    const auto where = scratch.path() / "rules.json";
    auto written = defaultGenerationRules();
    written.weights[0] = 5.0;

    ASSERT_FALSE(saveRulesFile(where, written).has_value());

    EXPECT_EQ(loadRulesFileOrDefaults(where, logger), written);
}

TEST(GenerationRulesTest, LoadRulesFileOrDefaults_WarnsWhenTheFileIsAbsent)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("rules.");

    EXPECT_CALL(logger, log(antwika::log::Level::Warning, _)).Times(1);

    EXPECT_EQ(
        loadRulesFileOrDefaults(scratch.path() / "absent.json", logger),
        defaultGenerationRules());
}

TEST(GenerationRulesTest, LoadRulesFileOrDefaults_WarnsWhenTheFileIsNotJson)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("rules.");
    scratch.write("broken.json", "{ not json");

    EXPECT_CALL(logger, log(antwika::log::Level::Warning, _)).Times(1);

    EXPECT_EQ(
        loadRulesFileOrDefaults(scratch.path() / "broken.json", logger),
        defaultGenerationRules());
}

TEST(GenerationRulesTest, LoadRulesFileOrDefaults_WarnsWhenTheRulesAreBad)
{
    NiceMock<MockLogger> logger;
    const ScratchDirectory scratch("rules.");
    scratch.write("wrong.json", "{\"weights\": 5}");

    EXPECT_CALL(logger, log(antwika::log::Level::Warning, _)).Times(1);

    EXPECT_EQ(
        loadRulesFileOrDefaults(scratch.path() / "wrong.json", logger),
        defaultGenerationRules());
}

TEST(GenerationRulesTest, SaveRulesFile_ReportsAPathItCannotOpen)
{
    const ScratchDirectory scratch("rules.");

    const auto failed = saveRulesFile(
        scratch.path() / "absent" / "rules.json",
        defaultGenerationRules());

    ASSERT_TRUE(failed.has_value());
    EXPECT_NE(failed->find("cannot open"), std::string::npos);
}

TEST(GenerationRulesTest, SaveRulesFile_ReportsAFileItCannotWrite)
{
    if (!std::filesystem::exists("/dev/full"))
    {
        GTEST_SKIP() << "no /dev/full to fill up";
    }

    const auto failed =
        saveRulesFile("/dev/full", defaultGenerationRules());

    ASSERT_TRUE(failed.has_value());
    EXPECT_NE(failed->find("cannot write"), std::string::npos);
}
