#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/Resource.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Walker.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    /**
     * @brief What every document of this format says it is.
     *
     * Checked before anything else is read, so a replay handed to the
     * loader is refused as the wrong kind of file rather than as a save
     * with every member missing.
     * That check matters more now that both formats state their version
     * in the same member: the magic is the only thing telling them
     * apart.
     */
    inline constexpr std::string_view kSaveMagic = "antwika-game-save";

    /**
     * @brief Which revision of the save format this build writes.
     *
     * An explicit integer from the first version rather than one added
     * once a second version was needed, because a document written
     * without one can only be dated by guessing.
     * A document that states no version at all is read as 1, which is
     * what makes this constant's first value free to assume.
     *
     * Stated in antwika::replay::kSchemaVersionKey -- "version" -- the
     * one member every persisted document in this code base carries its
     * version in, rather than a name of this format's own.
     */
    inline constexpr std::uint32_t kSaveFormatVersion = 2;

    /**
     * @brief Build the migration chain for the save document format.
     * @return A chain that brings a save document of any version this
     * build still reads up to kSaveFormatVersion.
     *
     * The save format's answer to standardReplayMigrations(), and the
     * whole reason MigrationChain is generic over the document: this
     * names its own list, its own current version and nothing else.
     * Empty today, because the format is still at version 1.
     * A factory rather than a constant, so that adding the first
     * migration changes one function and nothing else.
     */
    [[nodiscard]] MigrationChain standardSaveMigrations();

    /**
     * @brief A whole session, as a save file holds it.
     *
     * Everything here is simulation state: fold the same events onto a
     * loaded SaveGame and the run continues identically.
     * Nothing here is render state -- the camera looks like it might be,
     * and is not, for the reason Camera itself gives at length.
     *
     * Integers throughout, no floating point, so the value a run is
     * resumed from is exactly the value it was suspended at on every
     * toolchain.
     *
     * What this deliberately does **not** hold is anything that is not
     * state a tick loop reads: in-flight input (a pointer mid-drag, a
     * button still held), the replay cursor, and the recorder's history.
     * Loading therefore starts a session with no drag under way, which is
     * the honest answer -- a half-finished drag is a fact about a hand on
     * a mouse, not about the world.
     */
    /**
     * @brief One walker, as a file has to remember it.
     *
     * Richer than the WalkerView a summary carries, because a summary
     * describes what is on screen and a save has to bring a session back
     * exactly as it was -- a walker halfway home with a half-empty load
     * is not the same walker as a fresh one on the same cell.
     */
    struct SavedWalker
    {
        Cell at;
        Direction facing = Direction::East;
        WalkerKind kind = WalkerKind::Food;
        std::int32_t carried = 0;
        std::int32_t stepsUntilHome = kRoamingSteps;
        std::uint8_t ticksUntilStep = 0;

        /**
         * @brief Which saved building sent it, by index.
         *
         * **An index rather than the ecs::Entity it is in memory.**
         * EntityManager hands out ids from a monotonic counter and a
         * restore destroys and recreates every entity, so the recreated
         * building is a different id from the one that was saved and a
         * raw handle would name nothing at all on the way back in.
         */
        std::optional<std::size_t> home = std::nullopt;

        [[nodiscard]] bool operator==(const SavedWalker &other) const
            = default;
    };

    /**
     * @brief One building, as a file has to remember it.
     *
     * Every countdown is here rather than reset on load, because they
     * exist precisely so two buildings put up a tick apart do not drain,
     * risk and spawn in lockstep -- and starting them all from the same
     * number is exactly the lockstep they avoid.
     */
    struct SavedBuilding
    {
        Cell at;
        BuildingKind kind = BuildingKind::House;
        std::array<std::int32_t, kResourceCount> stock{};
        std::int32_t risk = 0;
        std::int32_t ticksUntilSpawn = 0;
        std::int32_t ticksUntilDrain = 0;
        std::int32_t ticksUntilRisk = 0;

        /** @brief Which saved walker it has out, by index. */
        std::optional<std::size_t> walker = std::nullopt;

        [[nodiscard]] bool operator==(const SavedBuilding &other) const
            = default;
    };

    struct SaveGame
    {
        /** @brief The plain app state: ticks folded so far, and score. */
        GameState state;

        /** @brief The bounds anything in this session may occupy. */
        GridExtent extent;

        /** @brief Where the grid is looked at from, and how closely. */
        Camera camera;

        /**
         * @brief Every cell carrying a path, in ascending Cell order.
         *
         * A vector rather than the PathIndex's set, matching what
         * GameSummary and SceneSnapshot already carry, so a save can be
         * taken straight from a summary.
         * pathIndexOf() turns it back into the index a run needs.
         *
         */
        std::vector<Cell> paths;

        /** @brief Every walker, in the world's own order. */
        std::vector<SavedWalker> walkers;

        /** @brief Every building, in the world's own order. */
        std::vector<SavedBuilding> buildings;

        /**
         * @brief The seed every generated part of the session came from.
         *
         * Carried from the first version even though nothing generates
         * anything yet, because a seed added later cannot be recovered
         * for a file written before it: the world it produced is already
         * on disk and the number that produced it is not.
         */
        std::uint64_t seed = 0;

        /**
         * @brief Compare two saves.
         * @param other The save to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(const SaveGame &other) const = default;
    };

    /**
     * @brief Encode a save as JSON matching the save-document schema.
     *
     * Pure: no filesystem, no clock, no ordering surprises. Split from
     * saveGameFile() exactly as replayToJson() is split from
     * saveReplayFile(), so a round trip can be asserted without a file.
     *
     * @param save The state to encode.
     * @return The encoded document, carrying kSaveFormatVersion.
     */
    [[nodiscard]] nlohmann::json saveGameToJson(const SaveGame &save);

    /**
     * @brief Decode a save from JSON, validating it first.
     *
     * Four stages, in this order: read the document's version, migrate it
     * up to kSaveFormatVersion, validate it against this version's
     * schema, then decode it.
     * The first two are standardSaveMigrations()'s, which also stamps the
     * version it brought the document to.
     * Validating after migrating rather than before is what lets one
     * schema describe the current version only.
     *
     * @param j The document to read.
     * @return The decoded state.
     * @throws SaveFormatError If j is not an object, states a version
     * this build cannot reach, fails the schema, or names a direction
     * that is not one of the four.
     */
    [[nodiscard]] SaveGame saveGameFromJson(const nlohmann::json &j);

    /**
     * @brief Take a save from a running session.
     *
     * Read from the World rather than from a GameSummary, because a
     * summary describes what is on screen and a save has to bring the
     * session back exactly as it was.
     * The loads, the countdowns and which building sent which walker are
     * none of them a picture.
     *
     * The building/walker link is written as a pair of indices into the
     * two arrays this produces, so a file names nothing that depends on
     * how one run happened to number its entities.
     *
     * @param world Read for the walkers and the buildings.
     * @param paths The roads to record.
     * @param camera Where the grid is looked at from.
     * @param state The plain app state to record.
     * @param extent The bounds the run was configured with.
     * @param seed The seed the run was configured with.
     * @return The state to write.
     */
    [[nodiscard]] SaveGame saveGameOf(
        const antwika::ecs::World &world,
        const PathIndex &paths,
        const Camera &camera,
        const GameState &state,
        GridExtent extent,
        std::uint64_t seed = 0);

    /**
     * @brief Rebuild the path lookup index a loaded session needs.
     * @param save The state to read the path cells from.
     * @return An index holding exactly those cells.
     */
    [[nodiscard]] PathIndex pathIndexOf(const SaveGame &save);

} // namespace antwika::game
