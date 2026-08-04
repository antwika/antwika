#include "antwika/game/ConfigFile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <antwika/config/AppConfigFile.hpp>
#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/ConfigFormatError.hpp>
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Resource.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::config::memberOr;
        using antwika::config::wholeShape;

        // A zero period would never come due, or divide by zero.
        // mouthsPerServing is one such divisor.
        // So the floor is one tick, stated beside the parse.
        nlohmann::json periodShape()
        {
            return wholeShape(
                1, std::numeric_limits<std::int32_t>::max());
        }

        // A cost may be nothing: free roads are a game somebody wants.
        // Never negative, which would pay the player to build.
        nlohmann::json costShape()
        {
            return wholeShape(
                0, std::numeric_limits<std::int64_t>::max());
        }

        // The bank may open in debt if a config says so.
        // Money is the one number here the game itself takes negative.
        nlohmann::json moneyShape()
        {
            return wholeShape(
                std::numeric_limits<std::int64_t>::min(),
                std::numeric_limits<std::int64_t>::max());
        }

        nlohmann::json costsShape()
        {
            nlohmann::json shape;
            shape["type"] = "object";
            shape["additionalProperties"] = false;

            // One property per kind, by the name a save file writes.
            // So a misspelt kind is refused by the schema.
            // And the refusal lists the names this build does know.
            for (std::size_t index = 0; index < kBuildingKindCount;
                 ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);
                shape["properties"]
                     [std::string(buildingKindName(kind))] = costShape();
            }

            return shape;

            // gcov puts the returned json's unwind block on this brace.
            // SaveGame.cpp's moneyShape() explains it at length.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        void describeMembers(nlohmann::json &schema)
        {
            schema["properties"]["startingMoney"] = moneyShape();
            schema["properties"]["roadCost"] = costShape();
            schema["properties"]["razeCost"] = costShape();
            schema["properties"]["buildingCosts"] = costsShape();
            schema["properties"]["riskPeriodTicks"] = periodShape();
            schema["properties"]["drainPeriodTicks"] = periodShape();
            schema["properties"]["mouthsPerServing"] = periodShape();
            schema["properties"]["spawnPeriodTicks"] = periodShape();
            schema["properties"]["burnDurationTicks"] = periodShape();
            schema["properties"]["settlerPeriodTicks"] = periodShape();
            schema["properties"]["evolvePeriodTicks"] = periodShape();
            schema["properties"]["devolvePeriodTicks"] = periodShape();
            schema["properties"]["productionPeriodTicks"] = periodShape();
            schema["properties"]["productionBatch"] = periodShape();
            schema["properties"]["labourPeriodTicks"] = periodShape();
            schema["properties"]["staffDecayPeriodTicks"] = periodShape();
            // One flag per resource, in Resource's own order.
            auto &staples = schema["properties"]["sustaining"];
            staples["type"] = "array";
            staples["items"]["type"] = "boolean";
            staples["minItems"] = kResourceCount;
            staples["maxItems"] = kResourceCount;

            schema["properties"]["walkerLimit"] = wholeShape(
                0, std::numeric_limits<std::int64_t>::max());
        }

        void encodeMembers(const GameConfig &config, nlohmann::json &out)
        {
            out["startingMoney"] = config.startingMoney;
            out["roadCost"] = config.roadCost;
            out["razeCost"] = config.razeCost;

            for (std::size_t index = 0; index < kBuildingKindCount;
                 ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);
                out["buildingCosts"]
                   [std::string(buildingKindName(kind))] =
                       config.costOf(kind);
            }
            out["riskPeriodTicks"] = config.riskPeriodTicks;
            out["drainPeriodTicks"] = config.drainPeriodTicks;
            out["mouthsPerServing"] = config.mouthsPerServing;
            out["spawnPeriodTicks"] = config.spawnPeriodTicks;
            out["burnDurationTicks"] = config.burnDurationTicks;
            out["settlerPeriodTicks"] = config.settlerPeriodTicks;
            out["evolvePeriodTicks"] = config.evolvePeriodTicks;
            out["devolvePeriodTicks"] = config.devolvePeriodTicks;
            out["productionPeriodTicks"] = config.productionPeriodTicks;
            out["productionBatch"] = config.productionBatch;
            out["labourPeriodTicks"] = config.labourPeriodTicks;
            out["staffDecayPeriodTicks"] = config.staffDecayPeriodTicks;
            out["walkerLimit"] = config.walkerLimit;

            for (const auto staple : config.sustaining)
            {
                out["sustaining"].push_back(staple);
            }
        }

        GameConfig decodeMembers(const nlohmann::json &document)
        {
            GameConfig config;
            config.startingMoney =
                memberOr(document, "startingMoney", config.startingMoney);
            config.roadCost =
                memberOr(document, "roadCost", config.roadCost);
            config.razeCost =
                memberOr(document, "razeCost", config.razeCost);

            if (document.contains("buildingCosts"))
            {
                const auto &costs = document.at("buildingCosts");

                for (std::size_t index = 0; index < kBuildingKindCount;
                     ++index)
                {
                    const auto kind = static_cast<BuildingKind>(index);
                    const auto name =
                        std::string(buildingKindName(kind));

                    if (costs.contains(name))
                    {
                        config.buildingCosts[index] =
                            costs.at(name).get<std::int64_t>();
                    }
                }
            }
            config.riskPeriodTicks =
                memberOr(document, "riskPeriodTicks", config.riskPeriodTicks);
            config.drainPeriodTicks =
                memberOr(document, "drainPeriodTicks", config.drainPeriodTicks);
            config.mouthsPerServing =
                memberOr(document, "mouthsPerServing", config.mouthsPerServing);
            config.spawnPeriodTicks =
                memberOr(document, "spawnPeriodTicks", config.spawnPeriodTicks);
            config.burnDurationTicks =
                memberOr(
                    document, "burnDurationTicks", config.burnDurationTicks);
            config.settlerPeriodTicks =
                memberOr(
                    document, "settlerPeriodTicks", config.settlerPeriodTicks);
            config.evolvePeriodTicks =
                memberOr(
                    document, "evolvePeriodTicks", config.evolvePeriodTicks);
            config.devolvePeriodTicks =
                memberOr(
                    document, "devolvePeriodTicks", config.devolvePeriodTicks);
            config.productionPeriodTicks =
                memberOr(
                    document,
                    "productionPeriodTicks",
                    config.productionPeriodTicks);
            config.productionBatch =
                memberOr(document, "productionBatch", config.productionBatch);
            config.labourPeriodTicks =
                memberOr(
                    document, "labourPeriodTicks", config.labourPeriodTicks);
            config.staffDecayPeriodTicks =
                memberOr(
                    document,
                    "staffDecayPeriodTicks",
                    config.staffDecayPeriodTicks);
            config.walkerLimit =
                memberOr(document, "walkerLimit", config.walkerLimit);

            if (document.contains("sustaining"))
            {
                const auto &staples = document.at("sustaining");

                for (std::size_t index = 0; index < kResourceCount; ++index)
                {
                    config.sustaining[index] =
                        staples.at(index).get<bool>();
                }
            }

            // A household that needs nothing can never empty.
            // That turns the population rule off rather than tuning it.
            if (std::none_of(
                    config.sustaining.begin(),
                    config.sustaining.end(),
                    [](const bool staple) { return staple; }))
            {
                throw antwika::config::ConfigFormatError(
                    "antwika::game: config names no good a household "
                    "cannot go without");
            }
            return config;
        }
    } // namespace

    ANTWIKA_CONFIG_FILE(
        "game",
        GameConfig,
        describeMembers,
        encodeMembers,
        decodeMembers)

} // namespace antwika::game
