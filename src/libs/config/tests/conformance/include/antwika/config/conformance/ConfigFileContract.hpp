#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include <unistd.h>

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include <antwika/app/AssetPath.hpp>
#include <antwika/config/ConfigFormatError.hpp>
#include <antwika/replay/SchemaVersion.hpp>

namespace antwika::config::conformance
{

    /**
     * @brief Every promise an application's config file makes, as one
     * test suite.
     *
     * Nine applications read a config through antwika::config, and
     * each made the same fourteen promises: the document states its
     * format and version, a value round trips, absence means the
     * defaults, the shipped file states them, and everything wrong is
     * refused rather than repaired.
     * Copied per application, the one contract that has to be right
     * was nine files that could drift apart; this is it said once,
     * instantiated per application the way MessageSetCompleteness is.
     *
     * A Traits supplies the application's half: its Config type, its
     * magic and version, its loader functions, a retuned() value with
     * every member off its default, an expectEqual() comparing member
     * by member, a floorMember() whose schema floor is one, and a
     * scratchPrefix() naming its temp directory.
     * Anything an application promises beyond these fourteen -- a
     * rule between two members, a partial-document table -- stays an
     * ordinary TEST in that application's own file.
     */
    template <typename Traits>
    class ConfigFileContract : public ::testing::Test
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

        // The pid keeps parallel ctest runs apart.
        // Each case is its own process under ctest -j.
        // See game/tests/ScratchDirectory.hpp for the long form.
        std::filesystem::path directory{
            std::filesystem::temp_directory_path()
            / (std::string(Traits::scratchPrefix()) + "."
               + std::to_string(::getpid()))};
    };

    TYPED_TEST_SUITE_P(ConfigFileContract);

    TYPED_TEST_P(ConfigFileContract, ADocumentStatesItsFormatAndVersion)
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

    TYPED_TEST_P(ConfigFileContract, AConfigRoundTripsThroughTheDocument)
    {
        TypeParam::expectEqual(
            TypeParam::fromJson(TypeParam::toJson(TypeParam::retuned())),
            TypeParam::retuned());
    }

    // A config stating one number is a one-line change.
    // Not a restatement of every default it leaves alone.
    TYPED_TEST_P(ConfigFileContract, AMinimalDocumentIsTheShippedApp)
    {
        nlohmann::json document;
        document["magic"] = std::string(TypeParam::magic());

        this->expectDefaults(TypeParam::fromJson(document));
    }

    TYPED_TEST_P(ConfigFileContract, AConfigRoundTripsThroughAStream)
    {
        std::stringstream stream;
        TypeParam::write(TypeParam::retuned(), stream);

        TypeParam::expectEqual(
            TypeParam::read(stream), TypeParam::retuned());
    }

    TYPED_TEST_P(ConfigFileContract, AConfigRoundTripsThroughAFile)
    {
        std::stringstream stream;
        TypeParam::write(TypeParam::retuned(), stream);
        this->writeText("config.json", stream.str());

        TypeParam::expectEqual(
            TypeParam::loadFileOrDefaults(this->pathIn("config.json")),
            TypeParam::retuned());
    }

    // An install nobody has tuned plays the shipped defaults.
    // That is a state, not a failure.
    TYPED_TEST_P(ConfigFileContract, AMissingFileIsTheShippedApp)
    {
        this->expectDefaults(
            TypeParam::loadFileOrDefaults(this->pathIn("nothing.json")));
    }

    // The file beside the executable is the one main() reads.
    // Pinned to the defaults, so shipping it changes nothing alone.
    TYPED_TEST_P(ConfigFileContract, TheShippedConfigStatesTheDefaults)
    {
        this->expectDefaults(TypeParam::loadFileOrDefaults(
            antwika::app::assetPath("config.json")));
        EXPECT_TRUE(std::filesystem::exists(
            antwika::app::assetPath("config.json")));
    }

    TYPED_TEST_P(ConfigFileContract, TextThatIsNotJsonIsRefused)
    {
        this->writeText("config.json", "not json at all");

        EXPECT_THROW(
            (void)TypeParam::loadFileOrDefaults(
                this->pathIn("config.json")),
            ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContract, ADocumentOfAnotherFormatIsRefused)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document["magic"] = "antwika-some-other-format";

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    // Read before anything is decoded.
    // So a file from a build this one has never met is refused.
    TYPED_TEST_P(ConfigFileContract, ADocumentFromANewerBuildIsRefused)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[this->versionKey()] = TypeParam::version() + 1;

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    TYPED_TEST_P(ConfigFileContract, ADocumentOfTheWrongShapeIsRefused)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[TypeParam::floorMember()] = "plenty";

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    // A value the member's meaning excludes is refused by the schema.
    TYPED_TEST_P(ConfigFileContract, AValueBelowTheFloorIsRefused)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[TypeParam::floorMember()] = 0;

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    // A misspelt member would otherwise be a change that never took.
    TYPED_TEST_P(ConfigFileContract, AnUnknownMemberIsRefused)
    {
        auto document = TypeParam::toJson(typename TypeParam::Config{});
        document[std::string(TypeParam::floorMember()) + "z"] = 9;

        EXPECT_THROW(
            (void)TypeParam::fromJson(document), ConfigFormatError);
    }

    // There has only ever been one revision; the chain is here anyway.
    // It is what refuses a document from a newer build.
    TYPED_TEST_P(ConfigFileContract, TheChainReachesTheCurrentVersion)
    {
        EXPECT_EQ(
            TypeParam::migrations().currentVersion(),
            TypeParam::version());
    }

    REGISTER_TYPED_TEST_SUITE_P(
        ConfigFileContract,
        ADocumentStatesItsFormatAndVersion,
        AConfigRoundTripsThroughTheDocument,
        AMinimalDocumentIsTheShippedApp,
        AConfigRoundTripsThroughAStream,
        AConfigRoundTripsThroughAFile,
        AMissingFileIsTheShippedApp,
        TheShippedConfigStatesTheDefaults,
        TextThatIsNotJsonIsRefused,
        ADocumentOfAnotherFormatIsRefused,
        ADocumentFromANewerBuildIsRefused,
        ADocumentOfTheWrongShapeIsRefused,
        AValueBelowTheFloorIsRefused,
        AnUnknownMemberIsRefused,
        TheChainReachesTheCurrentVersion);

} // namespace antwika::config::conformance
