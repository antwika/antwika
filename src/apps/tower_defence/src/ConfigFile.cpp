#include "antwika/tower_defence/ConfigFile.hpp"

#include <cstdint>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        constexpr antwika::config::Format kFormat{
            .magic = kConfigMagic, .version = kConfigFormatVersion};

        nlohmann::json configSchema()
        {
            auto schema = antwika::config::documentSchema(
                kFormat, "antwika tower_defence config document");
            schema["properties"]["startingLives"] =
                antwika::config::wholeShape(
                    1, std::numeric_limits<std::uint32_t>::max());
            schema["properties"]["framePeriodMs"] =
                antwika::config::wholeShape(
                    1, std::numeric_limits<std::int32_t>::max());
            return schema;
        } // GCOVR_EXCL_LINE

        const nlohmann::json_schema::json_validator &configValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                configSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    MigrationChain standardConfigMigrations()
    {
        // Every branch left on the excluded line is the allocator's.
        // The list is empty, so no heap branch is taken here.
        // What is left is the throw edge of building it.
        return MigrationChain({}, kConfigFormatVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json configToJson(const TowerDefenceConfig &config)
    {
        auto encoded = antwika::config::newDocument(kFormat);
        encoded["startingLives"] = config.startingLives;
        encoded["framePeriodMs"] = config.framePeriodMs;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    TowerDefenceConfig configFromJson(const nlohmann::json &document)
    {
        using antwika::config::memberOr;

        const auto brought = antwika::config::migrated(
            document,
            standardConfigMigrations(),
            configValidator(),
            "antwika::tower_defence: config JSON failed schema validation: ");

        TowerDefenceConfig config;
        config.startingLives =
            memberOr(brought, "startingLives", config.startingLives);
        config.framePeriodMs =
            memberOr(brought, "framePeriodMs", config.framePeriodMs);
        return config;
    }

    void writeConfig(const TowerDefenceConfig &config, std::ostream &out)
    {
        antwika::config::writeConfig(configToJson(config), out);
    }

    TowerDefenceConfig readConfig(std::istream &in)
    {
        return configFromJson(antwika::config::parseConfig(in));
    }

    TowerDefenceConfig loadConfigFileOrDefaults(const std::string &path)
    {
        const auto document = antwika::config::parseConfigFile(path);

        // A file that is not there is an install nobody has tuned.
        // Which is a state rather than a failure.
        return document.has_value() ? configFromJson(*document)
                                    : TowerDefenceConfig{};
    }

} // namespace antwika::tower_defence
