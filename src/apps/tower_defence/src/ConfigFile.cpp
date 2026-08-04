#include "antwika/tower_defence/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>

#include "antwika/tower_defence/MobKind.hpp"

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
            // One profile per kind, in MobKind's own order.
            auto &mobs = schema["properties"]["mobs"];
            mobs["type"] = "array";
            mobs["items"]["type"] = "object";
            mobs["items"]["additionalProperties"] = false;
            // The braced list's spare branches are the allocator's.
            // Every other required list here carries the same note.
            mobs["items"]["required"] = {
                "ticksPerCell",
                "health",
                "armour",
                "reward"}; // GCOVR_EXCL_LINE
            mobs["items"]["properties"]["ticksPerCell"] = wholeShape(
                1, std::numeric_limits<std::uint32_t>::max());
            mobs["items"]["properties"]["health"] = wholeShape(
                1, std::numeric_limits<std::uint32_t>::max());
            mobs["items"]["properties"]["armour"] = wholeShape(
                0, std::numeric_limits<std::uint32_t>::max());
            mobs["items"]["properties"]["reward"] = wholeShape(
                0, std::numeric_limits<std::uint32_t>::max());
            mobs["minItems"] = static_cast<std::int64_t>(kMobKindCount);
            mobs["maxItems"] = static_cast<std::int64_t>(kMobKindCount);

            schema["properties"]["framePeriodMs"] =
                wholeShape(1, std::numeric_limits<std::int32_t>::max());
        }

        void encodeMembers(
            const TowerDefenceConfig &config, nlohmann::json &out)
        {
            out["startingLives"] = config.startingLives;
            for (const auto mob : config.mobs)
            {
                nlohmann::json one;
                one["ticksPerCell"] = mob.ticksPerCell;
                one["health"] = mob.health;
                one["armour"] = mob.armour;
                one["reward"] = mob.reward;
                out["mobs"].push_back(std::move(one));
            }

            out["framePeriodMs"] = config.framePeriodMs;
        }

        TowerDefenceConfig decodeMembers(const nlohmann::json &document)
        {
            TowerDefenceConfig config;
            config.startingLives =
                memberOr(document, "startingLives", config.startingLives);
            config.framePeriodMs =
                memberOr(document, "framePeriodMs", config.framePeriodMs);

            if (document.contains("mobs"))
            {
                const auto &mobs = document.at("mobs");

                for (std::size_t index = 0; index < kMobKindCount; ++index)
                {
                    const auto &one = mobs.at(index);
                    config.mobs[index] = MobProfile{
                        .ticksPerCell =
                            one.at("ticksPerCell").get<std::uint32_t>(),
                        .health = one.at("health").get<std::int32_t>(),
                        .armour = one.at("armour").get<std::int32_t>(),
                        .reward = one.at("reward").get<std::uint64_t>()};
                }
            }
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
