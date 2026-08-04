#include "antwika/tower_defence/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["startingLives"] =
                wholeShape(
                    1, std::numeric_limits<std::uint32_t>::max());
            schema["properties"]["framePeriodMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(
            const TowerDefenceConfig &config, nlohmann::json &out)
        {
            out["startingLives"] = config.startingLives;
            out["framePeriodMs"] = config.framePeriodMs;
        }

        TowerDefenceConfig decodeMembers(const nlohmann::json &document)
        {
            TowerDefenceConfig config;
            config.startingLives =
                memberOr(document, "startingLives", config.startingLives);
            config.framePeriodMs =
                memberOr(document, "framePeriodMs", config.framePeriodMs);
            return config;
        }

        const FileFormat<TowerDefenceConfig> &fileFormat()
        {
            using AppFormat = FileFormat<TowerDefenceConfig>;

            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const AppFormat format(
                FormatSpec<TowerDefenceConfig>{
                    .format =
                        {.magic = kConfigMagic,
                         .version = kConfigFormatVersion},
                    .title = "antwika tower_defence config document",
                    .whatFailed =
                        "antwika::tower_defence: config JSON failed schema "
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

    nlohmann::json configToJson(const TowerDefenceConfig &config)
    {
        return fileFormat().toJson(config);
    }

    TowerDefenceConfig configFromJson(const nlohmann::json &document)
    {
        return fileFormat().fromJson(document);
    }

    void writeConfig(const TowerDefenceConfig &config, std::ostream &out)
    {
        fileFormat().write(config, out);
    }

    TowerDefenceConfig readConfig(std::istream &in)
    {
        return fileFormat().read(in);
    }

    TowerDefenceConfig loadConfigFileOrDefaults(const std::string &path)
    {
        return fileFormat().loadFileOrDefaults(path);
    }

} // namespace antwika::tower_defence
