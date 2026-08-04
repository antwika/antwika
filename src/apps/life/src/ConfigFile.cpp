#include "antwika/life/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::life
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["tickIntervalMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(const LifeConfig &config, nlohmann::json &out)
        {
            out["tickIntervalMs"] = config.tickIntervalMs;
        }

        LifeConfig decodeMembers(const nlohmann::json &document)
        {
            LifeConfig config;
            config.tickIntervalMs =
                memberOr(document, "tickIntervalMs", config.tickIntervalMs);
            return config;
        }

        const FileFormat<LifeConfig> &fileFormat()
        {
            using AppFormat = FileFormat<LifeConfig>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const AppFormat format(
                FormatSpec<LifeConfig>{
                    .format =
                        {.magic = kConfigMagic,
                         .version = kConfigFormatVersion},
                    .title = "antwika life config document",
                    .whatFailed =
                        "antwika::life: config JSON failed schema "
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

    nlohmann::json configToJson(const LifeConfig &config)
    {
        return fileFormat().toJson(config);
    }

    LifeConfig configFromJson(const nlohmann::json &document)
    {
        return fileFormat().fromJson(document);
    }

    void writeConfig(const LifeConfig &config, std::ostream &out)
    {
        fileFormat().write(config, out);
    }

    LifeConfig readConfig(std::istream &in)
    {
        return fileFormat().read(in);
    }

    LifeConfig loadConfigFileOrDefaults(const std::string &path)
    {
        return fileFormat().loadFileOrDefaults(path);
    }

} // namespace antwika::life
