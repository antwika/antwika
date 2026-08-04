#include "antwika/tower_defence/StateDump.hpp"

#include <cstddef>
#include <exception>
#include <string>
#include <utility>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/NameTable.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::replay::countShape;
        using antwika::replay::objectShape;
        using antwika::replay::wordShape;

        // The names a dump document holds, one per mob kind.
        // Persisted, so they may not change once written.
        constexpr antwika::replay::NameTable<MobKind, kMobKindCount>
            kMobKinds{{"grunt", "runner", "brute", "shielded"}};

        // The names a dump document holds, one per campaign phase.
        constexpr antwika::replay::NameTable<CampaignPhase, 3> kPhases{
            {"fighting", "won", "lost"}};

        nlohmann::json stateSchema()
        {
            // The level and the wave plan are validated by absence.
            // Both are regenerated from the seed on restore.
            // A dump carrying one is one this build never wrote.
            nlohmann::json schema = antwika::replay::documentShape(
                "antwika tower defence dump state",
                {"level",
                 "score",
                 "lives",
                 "ticks",
                 "phase",
                 "bestScore",
                 "battle"});

            auto &top = schema["properties"];
            for (const auto *counter :
                 {"level", "score", "lives", "ticks", "bestScore"})
            {
                top[counter] = countShape();
            }
            top["phase"] = wordShape();

            auto &battle = top["battle"];
            battle = objectShape(
                {"waveIndex",
                 "spawnedInWave",
                 "ticksUntilRelease",
                 "ticks",
                 "nextMobId",
                 "nextTowerId",
                 "mobs",
                 "towers"});

            auto &fight = battle["properties"];
            for (const auto *counter :
                 {"waveIndex",
                  "spawnedInWave",
                  "ticksUntilRelease",
                  "ticks",
                  "nextMobId",
                  "nextTowerId"})
            {
                fight[counter] = countShape();
            }

            fight["mobs"]["type"] = "array";
            fight["mobs"]["items"] = objectShape(
                {"id", "kind", "pathIndex", "health", "ticksUntilStep"});

            auto &mob = fight["mobs"]["items"];
            mob["properties"]["id"] = countShape();
            mob["properties"]["kind"] = wordShape();
            mob["properties"]["pathIndex"] = countShape();

            // At least one, since a dead mob is never in a dump.
            // A mob at zero died on the tick it got there.
            mob["properties"]["health"]["type"] = "integer";
            mob["properties"]["health"]["minimum"] = 1;
            mob["properties"]["ticksUntilStep"] = countShape();

            fight["towers"]["type"] = "array";
            fight["towers"]["items"] = objectShape({"id", "cell"});

            auto &tower = fight["towers"]["items"];
            tower["properties"]["id"] = countShape();

            auto &cell = tower["properties"]["cell"];
            cell = objectShape({"x", "y"});
            cell["properties"]["x"] = countShape();
            cell["properties"]["y"] = countShape();

            return schema;
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

        // Assignment style rather than a brace-init per entry.
        // An initializer list compiles to untakeable branches.
        battle["mobs"] = nlohmann::json::array();
        for (const Mob &mob : fight.mobs)
        {
            nlohmann::json entry;
            entry["id"] = mob.id;
            entry["kind"] = std::string(kMobKinds.name(mob.kind));
            entry["pathIndex"] = mob.pathIndex;
            entry["health"] = mob.health;
            entry["ticksUntilStep"] = mob.ticksUntilStep;
            battle["mobs"].push_back(std::move(entry));
        }

        battle["towers"] = nlohmann::json::array();
        for (const Tower &tower : fight.towers)
        {
            nlohmann::json entry;
            entry["id"] = tower.id;
            entry["cell"]["x"] = tower.cell.x;
            entry["cell"]["y"] = tower.cell.y;
            battle["towers"].push_back(std::move(entry));
        }

        nlohmann::json encoded;
        encoded["level"] = dump.campaign.level;
        encoded["score"] = dump.campaign.score;
        encoded["lives"] = dump.campaign.lives;
        encoded["ticks"] = dump.campaign.ticks;
        encoded["phase"] =
            std::string(kPhases.name(dump.campaign.phase));
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
            antwika::replay::validatorFor<stateSchema>().validate(
                state);
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
        const auto phase = kPhases.from(phaseNamed);

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
            const auto kind = kMobKinds.from(kindNamed);

            if (!kind.has_value())
            {
                throw StateDumpError(
                    "antwika::tower_defence: dump names a mob kind "
                    "this build does not know: "
                    + kindNamed);
            }

            // A named mob rather than a temporary in the call.
            // gcov parks a temporary's unwind code on its head line.
            Mob walker;
            walker.id = entry.at("id").get<std::uint32_t>();
            walker.kind = *kind;
            walker.pathIndex =
                entry.at("pathIndex").get<std::size_t>();
            walker.health = entry.at("health").get<std::int32_t>();
            walker.ticksUntilStep =
                entry.at("ticksUntilStep").get<std::uint32_t>();
            fight.mobs.push_back(walker);
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
