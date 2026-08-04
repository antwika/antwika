#include "antwika/game/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/Format.hpp>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    namespace
    {
        constexpr antwika::config::Format kFormat{
            .magic = kConfigMagic, .version = kConfigFormatVersion};

        // A zero period would never come due, or divide by zero.
        // mouthsPerServing is one such divisor.
        // So the floor is one tick, stated beside the parse.
        nlohmann::json periodShape()
        {
            return antwika::config::wholeShape(
                1, std::numeric_limits<std::int32_t>::max());
        }

        // A cost may be nothing: free roads are a game somebody wants.
        // Never negative, which would pay the player to build.
        nlohmann::json costShape()
        {
            return antwika::config::wholeShape(
                0, std::numeric_limits<std::int64_t>::max());
        }

        // The bank may open in debt if a config says so.
        // Money is the one number here the game itself takes negative.
        nlohmann::json moneyShape()
        {
            return antwika::config::wholeShape(
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

        nlohmann::json configSchema()
        {
            auto schema = antwika::config::documentSchema(
                kFormat, "antwika game config document");

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
            schema["properties"]["walkerLimit"] =
                antwika::config::wholeShape(
                    0, std::numeric_limits<std::int64_t>::max());
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

    nlohmann::json configToJson(const GameConfig &config)
    {
        auto encoded = antwika::config::newDocument(kFormat);

        encoded["startingMoney"] = config.startingMoney;
        encoded["roadCost"] = config.roadCost;
        encoded["razeCost"] = config.razeCost;

        for (std::size_t index = 0; index < kBuildingKindCount; ++index)
        {
            const auto kind = static_cast<BuildingKind>(index);
            encoded["buildingCosts"]
                   [std::string(buildingKindName(kind))] =
                       config.costOf(kind);
        }

        encoded["riskPeriodTicks"] = config.riskPeriodTicks;
        encoded["drainPeriodTicks"] = config.drainPeriodTicks;
        encoded["mouthsPerServing"] = config.mouthsPerServing;
        encoded["spawnPeriodTicks"] = config.spawnPeriodTicks;
        encoded["burnDurationTicks"] = config.burnDurationTicks;
        encoded["settlerPeriodTicks"] = config.settlerPeriodTicks;
        encoded["evolvePeriodTicks"] = config.evolvePeriodTicks;
        encoded["devolvePeriodTicks"] = config.devolvePeriodTicks;
        encoded["productionPeriodTicks"] = config.productionPeriodTicks;
        encoded["productionBatch"] = config.productionBatch;
        encoded["labourPeriodTicks"] = config.labourPeriodTicks;
        encoded["staffDecayPeriodTicks"] = config.staffDecayPeriodTicks;
        encoded["walkerLimit"] = config.walkerLimit;
        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    GameConfig configFromJson(const nlohmann::json &document)
    {
        using antwika::config::memberOr;

        const auto brought = antwika::config::migrated(
            document,
            standardConfigMigrations(),
            configValidator(),
            "antwika::game: config JSON failed schema validation: ");

        GameConfig config;
        config.startingMoney =
            memberOr(brought, "startingMoney", config.startingMoney);
        config.roadCost = memberOr(brought, "roadCost", config.roadCost);
        config.razeCost = memberOr(brought, "razeCost", config.razeCost);

        if (brought.contains("buildingCosts"))
        {
            const auto &costs = brought.at("buildingCosts");

            for (std::size_t index = 0; index < kBuildingKindCount;
                 ++index)
            {
                const auto kind = static_cast<BuildingKind>(index);
                const auto name = std::string(buildingKindName(kind));

                if (costs.contains(name))
                {
                    config.buildingCosts[index] =
                        costs.at(name).get<std::int64_t>();
                }
            }
        }

        config.riskPeriodTicks =
            memberOr(brought, "riskPeriodTicks", config.riskPeriodTicks);
        config.drainPeriodTicks = memberOr(
            brought, "drainPeriodTicks", config.drainPeriodTicks);
        config.mouthsPerServing = memberOr(
            brought, "mouthsPerServing", config.mouthsPerServing);
        config.spawnPeriodTicks = memberOr(
            brought, "spawnPeriodTicks", config.spawnPeriodTicks);
        config.burnDurationTicks = memberOr(
            brought, "burnDurationTicks", config.burnDurationTicks);
        config.settlerPeriodTicks = memberOr(
            brought, "settlerPeriodTicks", config.settlerPeriodTicks);
        config.evolvePeriodTicks = memberOr(
            brought, "evolvePeriodTicks", config.evolvePeriodTicks);
        config.devolvePeriodTicks = memberOr(
            brought, "devolvePeriodTicks", config.devolvePeriodTicks);
        config.productionPeriodTicks = memberOr(
            brought,
            "productionPeriodTicks",
            config.productionPeriodTicks);
        config.productionBatch = memberOr(
            brought, "productionBatch", config.productionBatch);
        config.labourPeriodTicks = memberOr(
            brought, "labourPeriodTicks", config.labourPeriodTicks);
        config.staffDecayPeriodTicks = memberOr(
            brought,
            "staffDecayPeriodTicks",
            config.staffDecayPeriodTicks);
        config.walkerLimit =
            memberOr(brought, "walkerLimit", config.walkerLimit);
        return config;
    }

    void writeConfig(const GameConfig &config, std::ostream &out)
    {
        antwika::config::writeConfig(configToJson(config), out);
    }

    GameConfig readConfig(std::istream &in)
    {
        return configFromJson(antwika::config::parseConfig(in));
    }

    GameConfig loadConfigFileOrDefaults(const std::string &path)
    {
        const auto document = antwika::config::parseConfigFile(path);

        // A file that is not there is a build nobody has rebalanced.
        // Which is a state rather than a failure.
        return document.has_value() ? configFromJson(*document)
                                    : GameConfig{};
    }

} // namespace antwika::game
