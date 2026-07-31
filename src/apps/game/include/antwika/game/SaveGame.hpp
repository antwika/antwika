#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GameSummary.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    /**
     * @brief What every document of this format says it is.
     *
     * Checked before anything else is read, so a replay handed to the
     * loader is refused as the wrong kind of file rather than as a save
     * with every member missing.
     */
    inline constexpr std::string_view kSaveMagic = "antwika-game-save";

    /**
     * @brief Which revision of the save format this build writes.
     *
     * An explicit integer from the first version rather than one added
     * once a second version was needed, because a document written
     * without one can only be dated by guessing.
     * A document that carries no schemaVersion at all is read as 1, which
     * is what makes this constant's first value free to assume.
     */
    inline constexpr std::uint32_t kSaveFormatVersion = 1;

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
         * When buildings other than paths exist, they belong beside this
         * as their own member and their own schema property.
         */
        std::vector<Cell> paths;

        /** @brief Every walker: where it is and which way it faces. */
        std::vector<WalkerView> walkers;

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
     * Validating after migrating rather than before is what lets one
     * schema describe the current version only.
     *
     * @param j The document to read.
     * @return The decoded state.
     * @throws SaveFormatError If j is not an object, carries a version
     * this build cannot reach, fails the schema, or names a direction
     * that is not one of the four.
     */
    [[nodiscard]] SaveGame saveGameFromJson(const nlohmann::json &j);

    /**
     * @brief Take a save from what a run amounted to.
     *
     * The integration point: bootstrap() already returns a GameSummary
     * holding the state, the paths, the walkers and the camera, so an app
     * that wants to save calls this and nothing else.
     *
     * @param summary What the run amounted to.
     * @param extent The bounds the run was configured with, which a
     * summary does not carry.
     * @param seed The seed the run was configured with.
     * @return The state to write.
     */
    [[nodiscard]] SaveGame saveGameOf(
        const GameSummary &summary,
        GridExtent extent,
        std::uint64_t seed = 0);

    /**
     * @brief Rebuild the path lookup index a loaded session needs.
     * @param save The state to read the path cells from.
     * @return An index holding exactly those cells.
     */
    [[nodiscard]] PathIndex pathIndexOf(const SaveGame &save);

} // namespace antwika::game
