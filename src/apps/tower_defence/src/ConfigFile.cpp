#include "antwika/tower_defence/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>

#include "antwika/tower_defence/MobKind.hpp"

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["startingLives"] =
                wholeShape(
                    1, std::numeric_limits<std::uint32_t>::max());
            auto &mobs = schema["properties"]["mobs"];
            mobs["type"] = "array";
            mobs["items"]["type"] = "object";
            mobs["items"]["additionalProperties"] = false;
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
    }

    ANTWIKA_CONFIG_FILE(
        "tower_defence",
        TowerDefenceConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

}
