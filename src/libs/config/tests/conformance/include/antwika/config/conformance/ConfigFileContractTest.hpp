#pragma once

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <unistd.h>

#include <antwika/app/AssetPath.hpp>
#include <antwika/config/ConfigFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/testing/ScratchPath.hpp>

namespace antwika::config::conformance
{

    template <typename Traits>
    class ConfigFileContractTest : public ::testing::Test
    {
    protected:
        using Config = typename Traits::Config;

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

        static void expectDefaults(const Config &decoded)
        {
            Traits::expectEqual(decoded, Config{});
        }

        [[nodiscard]] static std::string versionKey()
        {
            return std::string(antwika::replay::kSchemaVersionKey);
        }

        std::filesystem::path directory{
            antwika::testing::scratchPath(Traits::scratchPrefix())};
    };

    TYPED_TEST_SUITE_P(ConfigFileContractTest);

    TYPED_TEST_P(ConfigFileContractTest, ToJson_StatesTheFormatAndVersion)
    {
        const auto encoded =
            TypeParam::toJson(typename TypeParam::Config{});

        EXPECT_EQ(
            encoded.at("magic").template get<std::string>(),
            TypeParam::magic());
        EXPECT_EQ(
            encoded.at(this->versionKey())
                .template get<std::uint32_t>(),
            TypeParam::version());
    }

    TYPED_TEST_P(ConfigFileContractTest, FromJson_RoundTripsAConfig)
    {
        TypeParam::expectEqual(
            TypeParam::fromJson(TypeParam::toJson(TypeParam::retuned())),
            TypeParam::retuned());
    }

    TYPED_TEST_P(ConfigFileContractTest,
                 FromJson_FillsAMinimalDocumentWithDefaults)
    {
        nlohmann::json document;
        document["magic"] = std::string(TypeParam::magic());

        this->expectDefaults(TypeParam::fromJson(document));
    }

    TYPED_TEST_P(ConfigFileContractTest, Read_RoundTripsAConfigThroughAStream)
    {
        std::stringstream stream;
        TypeParam::write(TypeParam::retuned(), stream);

        TypeParam::expectEqual(
            TypeParam::read(stream), TypeParam::retuned());
    }

    TYPED_TEST_P(ConfigFileContractTest, LoadFileOrDefaults_RoundTripsAConfig)
    {
        std::stringstream stream;
        TypeParam::write(TypeParam::retuned(), stream);
        this->writeText("config.json", stream.str());

        TypeParam::expectEqual(
            TypeParam::loadFileOrDefaults(this->pathIn("config.json")),
            TypeParam::retuned());
    }

    TYPED_TEST_P(ConfigFileContractTest,
                 LoadFileOrDefaults_AnswersDefaultsForAMissingFile)
    {
        this->expectDefaults(
            TypeParam::loadFileOrDefaults(this->pathIn("nothing.json")));
    }

    TYPED_TEST_P(ConfigFileContractTest,
                 LoadFileOrDefaults_ReadsTheShippedDefaults)
    {
        this->expectDefaults(TypeParam::loadFileOrDefaults(
            antwika::app::assetPath("config.json")));
        EXPECT_TRUE(std::filesystem::exists(
            antwika::app::assetPath("config.json")));
    }

    TYPED_TEST_P(ConfigFileContractTest,
                 LoadFileOrDefaults_RefusesTextThatIsNotJson)
    {
        this->writeText("config.json", "not json at all");

        EXPECT_THROW(
            (void)TypeParam::loadFileOrDefaults(
                this->pathIn("config.json")),
            ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContractTest, FromJson_RefusesAnotherFormat)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document["magic"] = "antwika-some-other-format";

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContractTest, FromJson_RefusesANewerBuild)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[this->versionKey()] = TypeParam::version() + 1;

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContractTest, FromJson_RefusesTheWrongShape)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[TypeParam::floorMember()] = "plenty";

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContractTest, FromJson_RefusesAValueBelowTheFloor)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[TypeParam::floorMember()] = 0;

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContractTest, FromJson_RefusesAnUnknownMember)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[std::string(TypeParam::floorMember()) + "z"] = 9;

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContractTest, Migrations_ReachTheCurrentVersion)
    {
        EXPECT_EQ(
            TypeParam::migrations().currentVersion(),
            TypeParam::version());
    }

    REGISTER_TYPED_TEST_SUITE_P(
        ConfigFileContractTest,
        ToJson_StatesTheFormatAndVersion,
        FromJson_RoundTripsAConfig,
        FromJson_FillsAMinimalDocumentWithDefaults,
        Read_RoundTripsAConfigThroughAStream,
        LoadFileOrDefaults_RoundTripsAConfig,
        LoadFileOrDefaults_AnswersDefaultsForAMissingFile,
        LoadFileOrDefaults_ReadsTheShippedDefaults,
        LoadFileOrDefaults_RefusesTextThatIsNotJson,
        FromJson_RefusesAnotherFormat,
        FromJson_RefusesANewerBuild,
        FromJson_RefusesTheWrongShape,
        FromJson_RefusesAValueBelowTheFloor,
        FromJson_RefusesAnUnknownMember,
        Migrations_ReachTheCurrentVersion);

}
