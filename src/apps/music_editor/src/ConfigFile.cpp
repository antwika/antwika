#include "antwika/music_editor/ConfigFile.hpp"

#include <cstdint>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::music_editor
{

    namespace
    {
        constexpr antwika::config::Format kFormat{
            .magic = kConfigMagic, .version = kConfigFormatVersion};

        nlohmann::json configSchema()
        {
            auto schema = antwika::config::documentSchema(
                kFormat, "antwika music_editor config document");
            schema["properties"]["tickIntervalMs"] =
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

    nlohmann::json configToJson(const MusicEditorConfig &config)
    {
        auto encoded = antwika::config::newDocument(kFormat);
        encoded["tickIntervalMs"] = config.tickIntervalMs;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    MusicEditorConfig configFromJson(const nlohmann::json &document)
    {
        using antwika::config::memberOr;

        const auto brought = antwika::config::migrated(
            document,
            standardConfigMigrations(),
            configValidator(),
            "antwika::music_editor: config JSON failed schema validation: ");

        MusicEditorConfig config;
        config.tickIntervalMs =
            memberOr(brought, "tickIntervalMs", config.tickIntervalMs);
        return config;
    }

    void writeConfig(const MusicEditorConfig &config, std::ostream &out)
    {
        antwika::config::writeConfig(configToJson(config), out);
    }

    MusicEditorConfig readConfig(std::istream &in)
    {
        return configFromJson(antwika::config::parseConfig(in));
    }

    MusicEditorConfig loadConfigFileOrDefaults(const std::string &path)
    {
        const auto document = antwika::config::parseConfigFile(path);

        // A file that is not there is an install nobody has tuned.
        // Which is a state rather than a failure.
        return document.has_value() ? configFromJson(*document)
                                    : MusicEditorConfig{};
    }

} // namespace antwika::music_editor
