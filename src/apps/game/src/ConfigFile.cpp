#include "antwika/game/ConfigFile.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/ConfigFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        // Two spaces, one member a line.
        // Enough to diff a rebalance against the defaults it changes.
        constexpr int kIndent = 2;

        // A zero period would never come due, or divide by zero.
        // mouthsPerServing is one such divisor.
        // So the floor is one tick, stated beside the parse.
        nlohmann::json periodShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 1;
            shape["maximum"] = std::numeric_limits<std::int32_t>::max();
            return shape;

            // gcov puts the returned json's unwind block on this brace.
            // SaveGame.cpp's moneyShape() explains it at length.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        // A cost may be nothing: free roads are a game somebody wants.
        // Never negative, which would pay the player to build.
        // Bounded by what a std::int64_t holds.
        // get<T>() narrows anything wider in silence.
        nlohmann::json costShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = 0;
            shape["maximum"] = std::numeric_limits<std::int64_t>::max();
            return shape;
        } // GCOVR_EXCL_LINE

        // The bank may open in debt if a config says so.
        // Money is the one number here the game itself takes negative.
        nlohmann::json moneyShape()
        {
            nlohmann::json shape;
            shape["type"] = "integer";
            shape["minimum"] = std::numeric_limits<std::int64_t>::min();
            shape["maximum"] = std::numeric_limits<std::int64_t>::max();
            return shape;
        } // GCOVR_EXCL_LINE

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
        } // GCOVR_EXCL_LINE

        nlohmann::json configSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika game config document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // The version member is described but not required.
            // A document without one is read as version 1 instead.
            // By the time this runs the document has been migrated.
            // So the only version it may carry is the current one.
            schema["required"] = {"magic"}; // GCOVR_EXCL_LINE
            schema["properties"]["magic"]["const"] =
                std::string(kConfigMagic);
            schema["properties"][std::string(replay::kSchemaVersionKey)]
                  ["const"] = kConfigFormatVersion;

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
                replay::boundedCountShape(
                    std::numeric_limits<std::int64_t>::max());
            return schema;
        } // GCOVR_EXCL_LINE

        const nlohmann::json_schema::json_validator &configValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                configSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        // The schema above has already refused any wrong shape.
        // So an absent member is the only branch left to these.
        std::int32_t whole(
            const nlohmann::json &document,
            const char *name,
            std::int32_t fallback)
        {
            return document.contains(name)
                       ? document.at(name).get<std::int32_t>()
                       : fallback;
        }

        std::int64_t coin(
            const nlohmann::json &document,
            const char *name,
            std::int64_t fallback)
        {
            return document.contains(name)
                       ? document.at(name).get<std::int64_t>()
                       : fallback;
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
        nlohmann::json encoded;
        encoded["magic"] = std::string(kConfigMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kConfigFormatVersion;

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
        const auto migrated =
            replay::readVersionedDocument<ConfigFormatError>(
                document,
                standardConfigMigrations(),
                configValidator(),
                "antwika::game: config JSON failed schema validation: ");

        GameConfig config;
        config.startingMoney =
            coin(migrated, "startingMoney", config.startingMoney);
        config.roadCost = coin(migrated, "roadCost", config.roadCost);
        config.razeCost = coin(migrated, "razeCost", config.razeCost);

        if (migrated.contains("buildingCosts"))
        {
            const auto &costs = migrated.at("buildingCosts");

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
            whole(migrated, "riskPeriodTicks", config.riskPeriodTicks);
        config.drainPeriodTicks =
            whole(migrated, "drainPeriodTicks", config.drainPeriodTicks);
        config.mouthsPerServing =
            whole(migrated, "mouthsPerServing", config.mouthsPerServing);
        config.spawnPeriodTicks =
            whole(migrated, "spawnPeriodTicks", config.spawnPeriodTicks);
        config.burnDurationTicks =
            whole(migrated, "burnDurationTicks", config.burnDurationTicks);
        config.settlerPeriodTicks = whole(
            migrated, "settlerPeriodTicks", config.settlerPeriodTicks);
        config.evolvePeriodTicks =
            whole(migrated, "evolvePeriodTicks", config.evolvePeriodTicks);
        config.devolvePeriodTicks = whole(
            migrated, "devolvePeriodTicks", config.devolvePeriodTicks);
        config.productionPeriodTicks = whole(
            migrated,
            "productionPeriodTicks",
            config.productionPeriodTicks);
        config.productionBatch =
            whole(migrated, "productionBatch", config.productionBatch);
        config.labourPeriodTicks =
            whole(migrated, "labourPeriodTicks", config.labourPeriodTicks);
        config.staffDecayPeriodTicks = whole(
            migrated,
            "staffDecayPeriodTicks",
            config.staffDecayPeriodTicks);

        if (migrated.contains("walkerLimit"))
        {
            config.walkerLimit =
                migrated.at("walkerLimit").get<std::size_t>();
        }

        return config;
    }

    void writeConfig(const GameConfig &config, std::ostream &out)
    {
        out << configToJson(config).dump(kIndent) << '\n';
    }

    GameConfig readConfig(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw ConfigFormatError(
                std::string("antwika::game: config is not valid JSON: ")
                + error.what());
        }

        return configFromJson(document);
    }

    GameConfig loadConfigFileOrDefaults(const std::string &path)
    {
        std::ifstream file(path);

        // A file that is not there is a build nobody has rebalanced.
        // Which is a state rather than a failure.
        if (!file.is_open())
        {
            return GameConfig{};
        }

        return readConfig(file);
    }

} // namespace antwika::game
