#include "antwika/atlas_editor/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::atlas_editor
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["framePeriodMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(const AtlasEditorConfig &config, nlohmann::json &out)
        {
            out["framePeriodMs"] = config.framePeriodMs;
        }

        AtlasEditorConfig decodeMembers(const nlohmann::json &document)
        {
            AtlasEditorConfig config;
            config.framePeriodMs =
                memberOr(document, "framePeriodMs", config.framePeriodMs);
            return config;
        }

        const FileFormat<AtlasEditorConfig> &fileFormat()
        {
            using AppFormat = FileFormat<AtlasEditorConfig>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const AppFormat format(
                FormatSpec<AtlasEditorConfig>{
                    .format =
                        {.magic = kConfigMagic,
                         .version = kConfigFormatVersion},
                    .title = "antwika atlas_editor config document",
                    .whatFailed =
                        "antwika::atlas_editor: config JSON failed schema "
                        "validation: ",
                    .members = describeMembers,
                    .encode = encodeMembers,
                    .decode = decodeMembers,
                    .migrations = standardConfigMigrations}); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    MigrationChain standardConfigMigrations()
    {
        // Every branch left on the excluded line is the allocator's.
        // The list is empty, so no heap branch is taken here.
        // What is left is the throw edge of building it.
        return MigrationChain({}, kConfigFormatVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json configToJson(const AtlasEditorConfig &config)
    {
        return fileFormat().toJson(config);
    }

    AtlasEditorConfig configFromJson(const nlohmann::json &document)
    {
        return fileFormat().fromJson(document);
    }

    void writeConfig(const AtlasEditorConfig &config, std::ostream &out)
    {
        fileFormat().write(config, out);
    }

    AtlasEditorConfig readConfig(std::istream &in)
    {
        return fileFormat().read(in);
    }

    AtlasEditorConfig loadConfigFileOrDefaults(const std::string &path)
    {
        return fileFormat().loadFileOrDefaults(path);
    }

} // namespace antwika::atlas_editor
