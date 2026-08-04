#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <unistd.h>

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/config/ConfigDocument.hpp"
#include "antwika/config/ConfigFormatError.hpp"
#include "antwika/config/Format.hpp"

using antwika::config::ConfigFormatError;
using antwika::config::documentSchema;
using antwika::config::Format;
using antwika::config::memberOr;
using antwika::config::migrated;
using antwika::config::newDocument;
using antwika::config::parseConfig;
using antwika::config::parseConfigFile;
using antwika::config::wholeShape;
using antwika::config::writeConfig;
using antwika::replay::MigrationChain;

namespace
{
    constexpr Format kFormat{
        .magic = "antwika-test-config", .version = 1};

    // One property beside the envelope.
    // So the schema tests can tell a member from a document refusal.
    [[nodiscard]] nlohmann::json schemaWithAPeriod()
    {
        auto schema = documentSchema(kFormat, "test config document");
        schema["properties"]["periodTicks"] = wholeShape(1, 10);
        return schema;
    }

    [[nodiscard]] const nlohmann::json_schema::json_validator &
    testValidator()
    {
        static const nlohmann::json_schema::json_validator validator(
            schemaWithAPeriod()); // GCOVR_EXCL_LINE
        return validator;
    }

    [[nodiscard]] nlohmann::json broughtUp(const nlohmann::json &document)
    {
        return migrated(
            document,
            MigrationChain({}, kFormat.version),
            testValidator(),
            "antwika::config tests: ");
    }

    class ConfigDocumentTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            std::filesystem::create_directories(directory);
        }

        void TearDown() override
        {
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
        }

        [[nodiscard]] std::string pathIn(const std::string &name) const
        {
            return (directory / name).string();
        }

        void writeText(const std::string &name, const std::string &text)
        {
            std::ofstream file(pathIn(name));
            file << text;
        }

        // Named per process, for ScratchDirectory.hpp's reason.
        // CTest runs every case as its own process.
        // A never-before-seen path cannot be mid-removal already.
        std::filesystem::path directory{
            std::filesystem::temp_directory_path()
            / ("antwika-config." + std::to_string(::getpid()))};
    };
} // namespace

TEST_F(ConfigDocumentTest, NewDocument_StatesMagicAndVersion)
{
    const auto document = newDocument(kFormat);

    EXPECT_EQ(
        document.at("magic").get<std::string>(), kFormat.magic);
    EXPECT_EQ(
        document.at("version").get<std::uint32_t>(), kFormat.version);
}

TEST_F(ConfigDocumentTest, Migrated_PassesADocumentOfTheFormat)
{
    auto document = newDocument(kFormat);
    document["periodTicks"] = 5;

    EXPECT_EQ(broughtUp(document), document);
}

TEST_F(ConfigDocumentTest, Migrated_RefusesAnotherFormatsMagic)
{
    auto document = newDocument(kFormat);
    document["magic"] = "antwika-game-save";

    EXPECT_THROW((void)broughtUp(document), ConfigFormatError);
}

// Read before anything is decoded.
// So a file from a build this one has never met is refused.
// Rather than read for happening to satisfy today's schema.
TEST_F(ConfigDocumentTest, Migrated_RefusesADocumentFromANewerBuild)
{
    auto document = newDocument(kFormat);
    document["version"] = kFormat.version + 1;

    EXPECT_THROW((void)broughtUp(document), ConfigFormatError);
}

// A misspelt member silently skipped would be a change that never took.
TEST_F(ConfigDocumentTest, Migrated_RefusesAnUnknownMember)
{
    auto document = newDocument(kFormat);
    document["periodTick"] = 5;

    EXPECT_THROW((void)broughtUp(document), ConfigFormatError);
}

TEST_F(ConfigDocumentTest, WholeShape_RefusesBelowTheMinimum)
{
    auto document = newDocument(kFormat);
    document["periodTicks"] = 0;

    EXPECT_THROW((void)broughtUp(document), ConfigFormatError);
}

TEST_F(ConfigDocumentTest, WholeShape_RefusesAboveTheMaximum)
{
    auto document = newDocument(kFormat);
    document["periodTicks"] = 11;

    EXPECT_THROW((void)broughtUp(document), ConfigFormatError);
}

TEST_F(ConfigDocumentTest, MemberOr_ReadsAPresentMember)
{
    auto document = newDocument(kFormat);
    document["periodTicks"] = 5;
    document["cost"] = 7;
    document["cap"] = 9;

    EXPECT_EQ(
        memberOr<std::int32_t>(document, "periodTicks", 1), 5);
    EXPECT_EQ(memberOr<std::int64_t>(document, "cost", 1), 7);
    EXPECT_EQ(memberOr<std::size_t>(document, "cap", 1U), 9U);
}

TEST_F(ConfigDocumentTest, MemberOr_FallsBackWhenAbsent)
{
    const auto document = newDocument(kFormat);

    EXPECT_EQ(
        memberOr<std::int32_t>(document, "periodTicks", 25), 25);
    EXPECT_EQ(memberOr<std::int64_t>(document, "cost", 4), 4);
    EXPECT_EQ(memberOr<std::size_t>(document, "cap", 64U), 64U);
}

TEST_F(ConfigDocumentTest, ADocumentRoundTripsThroughAStream)
{
    auto document = newDocument(kFormat);
    document["periodTicks"] = 5;

    std::stringstream stream;
    writeConfig(document, stream);

    EXPECT_EQ(parseConfig(stream), document);
}

TEST_F(ConfigDocumentTest, ParseConfig_RefusesTextThatIsNotJson)
{
    std::stringstream stream("not json at all");

    EXPECT_THROW((void)parseConfig(stream), ConfigFormatError);
}

// A missing file is the caller's defaults, whatever those are.
// That is a state rather than a failure, so it is nothing, not a throw.
TEST_F(ConfigDocumentTest, ParseConfigFile_AnswersNothingForAMissingFile)
{
    EXPECT_FALSE(
        parseConfigFile(pathIn("nothing-here.json")).has_value());
}

TEST_F(ConfigDocumentTest, ParseConfigFile_ReadsAPresentFile)
{
    auto document = newDocument(kFormat);
    document["periodTicks"] = 5;

    std::stringstream stream;
    writeConfig(document, stream);
    writeText("config.json", stream.str());

    EXPECT_EQ(parseConfigFile(pathIn("config.json")), document);
}

TEST_F(ConfigDocumentTest, ParseConfigFile_RefusesAFileThatIsNotJson)
{
    writeText("config.json", "not json at all");

    EXPECT_THROW(
        (void)parseConfigFile(pathIn("config.json")),
        ConfigFormatError);
}
