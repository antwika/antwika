#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>

#include <antwika/replay/MigrationChain.hpp>
#include <antwika/testing/ScratchPath.hpp>

#include "antwika/config/ConfigDocument.hpp"
#include "antwika/config/ConfigFormatError.hpp"
#include "antwika/config/FileFormat.hpp"

using antwika::config::ConfigFormatError;
using antwika::config::FileFormat;
using antwika::config::FormatSpec;
using antwika::config::memberOr;
using antwika::config::wholeShape;

namespace
{
    constexpr std::int32_t kDefaultPeriodTicks = 3;

    struct TestConfig final
    {
        std::int32_t periodTicks = kDefaultPeriodTicks;
    };

    void describeMembers(nlohmann::json &schema)
    {
        schema["properties"]["periodTicks"] = wholeShape(1, 10);
    }

    void encodeMembers(const TestConfig &config, nlohmann::json &encoded)
    {
        encoded["periodTicks"] = config.periodTicks;
    }

    TestConfig decodeMembers(const nlohmann::json &document)
    {
        return TestConfig{
            .periodTicks = memberOr<std::int32_t>(
                document, "periodTicks", kDefaultPeriodTicks)};
    }

    antwika::replay::MigrationChain noMigrations()
    {
        return antwika::replay::MigrationChain({}, 1);
    }

    [[nodiscard]] FormatSpec<TestConfig> testSpec()
    {
        return FormatSpec<TestConfig>{
            .format =
                {.magic = "antwika-test-file-format", .version = 1},
            .title = "antwika test file format document",
            .whatFailed = "antwika::config tests: ",
            .members = describeMembers,
            .encode = encodeMembers,
            .decode = decodeMembers,
            .migrations = noMigrations};
    }
}

TEST(FileFormatTest, LoadFile_RefusesAFileThatIsNotThere)
{
    const antwika::testing::ScratchFile file(
        "antwika_config_file_format_absent.json");
    const FileFormat<TestConfig> format(testSpec());

    try
    {
        (void)format.loadFile(file.string());
        FAIL() << "the missing file should have ended the load";
    }
    catch (const ConfigFormatError &failed)
    {
        EXPECT_EQ(
            std::string(failed.what()),
            "antwika: no such file to read: " + file.string());
    }
}

TEST(FileFormatTest, LoadFileIfPresent_AnswersNothingForAMissingFile)
{
    const antwika::testing::ScratchFile file(
        "antwika_config_file_format_nothing.json");
    const FileFormat<TestConfig> format(testSpec());

    EXPECT_FALSE(format.loadFileIfPresent(file.string()).has_value());
}

TEST(FileFormatTest, LoadFileIfPresent_ReadsWhatStoreFileLeftBehind)
{
    const antwika::testing::ScratchFile file(
        "antwika_config_file_format_stored.json");
    const FileFormat<TestConfig> format(testSpec());

    format.storeFile(TestConfig{.periodTicks = 7}, file.string());

    const auto loaded = format.loadFileIfPresent(file.string());

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->periodTicks, 7);
}

TEST(FileFormatTest, LoadFile_RefusesAValueOutsideTheShapeItDescribed)
{
    const antwika::testing::ScratchFile file(
        "antwika_config_file_format_refused.json");
    const FileFormat<TestConfig> format(testSpec());

    file.write(
        R"({"magic":"antwika-test-file-format","periodTicks":11,)"
        R"("version":1})");

    EXPECT_THROW(
        (void)format.loadFile(file.string()), ConfigFormatError);
}
