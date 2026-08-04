#include "antwika/tower_defence/StateDump.hpp"

#include <array>
#include <cstddef>
#include <exception>
#include <optional>
#include <string>

#include <nlohmann/json-schema.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        // The names a dump document holds, one per mob kind.
        // Persisted, so they may not change once written.
        constexpr std::array<std::string_view, kMobKindCount>
            kMobKindNames{"grunt", "runner", "brute", "shielded"};

        // The names a dump document holds, one per campaign phase.
        constexpr std::size_t kPhaseCount = 3;
        constexpr std::array<std::string_view, kPhaseCount> kPhaseNames{
            "fighting", "won", "lost"};

        [[nodiscard]] std::string_view mobKindName(
            const MobKind kind) noexcept
        {
            return kMobKindNames[
                static_cast<std::size_t>(kind) % kMobKindCount];
        }

        [[nodiscard]] std::optional<MobKind> mobKindFromName(
            const std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kMobKindCount; ++index)
            {
                if (kMobKindNames[index] == name)
                {
                    return static_cast<MobKind>(index);
                }
            }

            return std::nullopt;
        }

        [[nodiscard]] std::string_view phaseName(
            const CampaignPhase phase) noexcept
        {
            return kPhaseNames[
                static_cast<std::size_t>(phase) % kPhaseCount];
        }

        [[nodiscard]] std::optional<CampaignPhase> phaseFromName(
            const std::string_view name) noexcept
        {
            for (std::size_t index = 0; index < kPhaseCount; ++index)
            {
                if (kPhaseNames[index] == name)
                {
                    return static_cast<CampaignPhase>(index);
                }
            }

            return std::nullopt;
        }

        nlohmann::json stateSchema()
        {
            // The level and the wave plan are validated by absence.
            // Both are regenerated from the seed on restore.
            // A dump carrying one is one this build never wrote.
            nlohmann::json schema;
            schema["$schema"] =
                "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika tower defence dump state";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {
                "level",
                "score",
                "lives",
                "ticks",
                "phase",
                "bestScore",
                "battle"}; // GCOVR_EXCL_LINE

            auto &top = schema["properties"];
            top["level"] = {{"type", "integer"}, {"minimum", 0}};
            top["score"] = {{"type", "integer"}, {"minimum", 0}};
            top["lives"] = {{"type", "integer"}, {"minimum", 0}};
            top["ticks"] = {{"type", "integer"}, {"minimum", 0}};
            top["phase"]["type"] = "string";
            top["bestScore"] = {{"type", "integer"}, {"minimum", 0}};

            auto &battle = top["battle"];
            battle["type"] = "object";
            battle["additionalProperties"] = false;
            battle["required"] = {
                "waveIndex",
                "spawnedInWave",
                "ticksUntilRelease",
                "ticks",
                "nextMobId",
                "nextTowerId",
                "mobs",
                "towers"}; // GCOVR_EXCL_LINE

            auto &fight = battle["properties"];
            for (const auto *counter :
                 {"waveIndex",
                  "spawnedInWave",
                  "ticksUntilRelease",
                  "ticks",
                  "nextMobId",
                  "nextTowerId"})
            {
                fight[counter] = {{"type", "integer"}, {"minimum", 0}};
            }

            auto &mob = fight["mobs"]["items"];
            fight["mobs"]["type"] = "array";
            mob["type"] = "object";
            mob["additionalProperties"] = false;
            mob["required"] = {
                "id",
                "kind",
                "pathIndex",
                "health",
                "ticksUntilStep"}; // GCOVR_EXCL_LINE
            mob["properties"]["id"] = {
                {"type", "integer"}, {"minimum", 0}};
            mob["properties"]["kind"]["type"] = "string";
            mob["properties"]["pathIndex"] = {
                {"type", "integer"}, {"minimum", 0}};

            // At least one, since a dead mob is never in a dump.
            // A mob at zero died on the tick it got there.
            mob["properties"]["health"] = {
                {"type", "integer"}, {"minimum", 1}};
            mob["properties"]["ticksUntilStep"] = {
                {"type", "integer"}, {"minimum", 0}};

            auto &tower = fight["towers"]["items"];
            fight["towers"]["type"] = "array";
            tower["type"] = "object";
            tower["additionalProperties"] = false;
            tower["required"] = {"id", "cell"}; // GCOVR_EXCL_LINE
            tower["properties"]["id"] = {
                {"type", "integer"}, {"minimum", 0}};

            auto &cell = tower["properties"]["cell"];
            cell["type"] = "object";
            cell["additionalProperties"] = false;
            cell["required"] = {"x", "y"}; // GCOVR_EXCL_LINE
            cell["properties"]["x"] = {
                {"type", "integer"}, {"minimum", 0}};
            cell["properties"]["y"] = {
                {"type", "integer"}, {"minimum", 0}};

            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const nlohmann::json_schema::json_validator validator(
                stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        // Version 1 is the first, so there is no step to take yet.
        return antwika::replay::MigrationChain({}, kStateDumpVersion);
    }

    nlohmann::json stateDumpToJson(const StateDump &dump)
    {
        nlohmann::json battle;
        const BattleMemory &fight = dump.campaign.battle;

        battle["waveIndex"] = fight.waveIndex;
        battle["spawnedInWave"] = fight.spawnedInWave;
        battle["ticksUntilRelease"] = fight.ticksUntilRelease;
        battle["ticks"] = fight.tickCount;
        battle["nextMobId"] = fight.nextMobId;
        battle["nextTowerId"] = fight.nextTowerId;

        battle["mobs"] = nlohmann::json::array();
        for (const Mob &mob : fight.mobs)
        {
            battle["mobs"].push_back(
                {{"id", mob.id},
                 {"kind", std::string(mobKindName(mob.kind))},
                 {"pathIndex", mob.pathIndex},
                 {"health", mob.health},
                 {"ticksUntilStep", mob.ticksUntilStep}});
        }

        battle["towers"] = nlohmann::json::array();
        for (const Tower &tower : fight.towers)
        {
            battle["towers"].push_back(
                {{"id", tower.id},
                 {"cell",
                  {{"x", tower.cell.x}, {"y", tower.cell.y}}}});
        }

        nlohmann::json encoded;
        encoded["level"] = dump.campaign.level;
        encoded["score"] = dump.campaign.score;
        encoded["lives"] = dump.campaign.lives;
        encoded["ticks"] = dump.campaign.ticks;
        encoded["phase"] = std::string(phaseName(dump.campaign.phase));
        encoded["bestScore"] = dump.bestScore;
        encoded["battle"] = std::move(battle);

        return encoded;

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    StateDump stateDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is StateDumpError.
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw StateDumpError(
                std::string(
                    "antwika::tower_defence: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        StateDump dump;
        dump.campaign.level = state.at("level").get<std::size_t>();
        dump.campaign.score = state.at("score").get<std::uint64_t>();
        dump.campaign.lives = state.at("lives").get<std::uint32_t>();
        dump.campaign.ticks = state.at("ticks").get<std::uint64_t>();
        dump.bestScore = state.at("bestScore").get<std::uint64_t>();

        const auto phaseNamed = state.at("phase").get<std::string>();
        const auto phase = phaseFromName(phaseNamed);

        if (!phase.has_value())
        {
            throw StateDumpError(
                "antwika::tower_defence: dump names a phase this "
                "build does not know: "
                + phaseNamed);
        }

        dump.campaign.phase = *phase;

        const nlohmann::json &battle = state.at("battle");
        BattleMemory &fight = dump.campaign.battle;

        fight.waveIndex =
            battle.at("waveIndex").get<std::size_t>();
        fight.spawnedInWave =
            battle.at("spawnedInWave").get<std::size_t>();
        fight.ticksUntilRelease =
            battle.at("ticksUntilRelease").get<std::uint64_t>();
        fight.tickCount = battle.at("ticks").get<std::uint64_t>();
        fight.nextMobId = battle.at("nextMobId").get<std::uint32_t>();
        fight.nextTowerId =
            battle.at("nextTowerId").get<std::uint32_t>();

        for (const nlohmann::json &entry : battle.at("mobs"))
        {
            const auto kindNamed = entry.at("kind").get<std::string>();
            const auto kind = mobKindFromName(kindNamed);

            if (!kind.has_value())
            {
                throw StateDumpError(
                    "antwika::tower_defence: dump names a mob kind "
                    "this build does not know: "
                    + kindNamed);
            }

            fight.mobs.push_back(Mob{
                .id = entry.at("id").get<std::uint32_t>(),
                .kind = *kind,
                .pathIndex = entry.at("pathIndex").get<std::size_t>(),
                .health = entry.at("health").get<std::int32_t>(),
                .ticksUntilStep =
                    entry.at("ticksUntilStep").get<std::uint32_t>()});
        }

        for (const nlohmann::json &entry : battle.at("towers"))
        {
            const nlohmann::json &cell = entry.at("cell");
            fight.towers.push_back(Tower{
                .id = entry.at("id").get<std::uint32_t>(),
                .cell = Cell{
                    .x = cell.at("x").get<std::uint32_t>(),
                    .y = cell.at("y").get<std::uint32_t>()}});
        }

        return dump;

        // gcov puts the returned value's unwind block here.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

} // namespace antwika::tower_defence
