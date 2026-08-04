#include "antwika/poker/ConfigFile.hpp"

#include <cstdint>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/ConfigFormatError.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::poker
{

    namespace
    {
        constexpr antwika::config::Format kFormat{
            .magic = kConfigMagic, .version = kConfigFormatVersion};

        // A blind or a buy-in of nothing is no cash game at all.
        // So the floor is one chip, stated beside the parse.
        nlohmann::json chipsShape()
        {
            return antwika::config::wholeShape(
                1, std::numeric_limits<std::int64_t>::max());
        }

        nlohmann::json configSchema()
        {
            auto schema = antwika::config::documentSchema(
                kFormat, "antwika poker config document");
            schema["properties"]["smallBlind"] = chipsShape();
            schema["properties"]["bigBlind"] = chipsShape();
            schema["properties"]["minimumBuyIn"] = chipsShape();
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

    nlohmann::json configToJson(const RoomConfig &config)
    {
        auto encoded = antwika::config::newDocument(kFormat);
        encoded["smallBlind"] = config.blinds.small;
        encoded["bigBlind"] = config.blinds.big;
        encoded["minimumBuyIn"] = config.minimumBuyIn;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    RoomConfig configFromJson(const nlohmann::json &document)
    {
        using antwika::config::memberOr;

        const auto brought = antwika::config::migrated(
            document,
            standardConfigMigrations(),
            configValidator(),
            "antwika::poker: config JSON failed schema validation: ");

        RoomConfig config;
        config.blinds.small =
            memberOr(brought, "smallBlind", config.blinds.small);
        config.blinds.big =
            memberOr(brought, "bigBlind", config.blinds.big);
        config.minimumBuyIn =
            memberOr(brought, "minimumBuyIn", config.minimumBuyIn);

        // The schema checks each number alone.
        // These two rules are between numbers.
        // So they live here beside the decode.
        if (config.blinds.big < config.blinds.small)
        {
            throw antwika::config::ConfigFormatError(
                "antwika::poker: config states a big blind smaller "
                "than the small blind");
        }

        if (config.minimumBuyIn < config.blinds.big)
        {
            throw antwika::config::ConfigFormatError(
                "antwika::poker: config states a minimum buy-in "
                "smaller than the big blind");
        }

        return config;
    }

    void writeConfig(const RoomConfig &config, std::ostream &out)
    {
        antwika::config::writeConfig(configToJson(config), out);
    }

    RoomConfig readConfig(std::istream &in)
    {
        return configFromJson(antwika::config::parseConfig(in));
    }

    RoomConfig loadConfigFileOrDefaults(const std::string &path)
    {
        const auto document = antwika::config::parseConfigFile(path);

        // A file that is not there is an install nobody has tuned.
        // Which is a state rather than a failure.
        // An if rather than a ternary, since RoomConfig has a string.
        // A mixed ternary's temporary hands gcov edges nobody reaches.
        if (!document.has_value())
        {
            // The unreached arm allocates the default tableName.
            // A seven-char literal's SSO always spares it.
            // See docs/confirming-unreachable-branches.md.
            return RoomConfig{}; // GCOVR_EXCL_LINE
        }

        return configFromJson(*document);
    }

} // namespace antwika::poker
