#pragma once

#include <cstdint>

#include <antwika/ecs/World.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief The one route between a running session and a save file.
     *
     * Both ways in exist and neither is a special case: `--load` restores
     * through this before the loop starts, and the Load button restores
     * through it from inside the tick path. One class rather than two
     * code paths, so a session resumed from the command line and one
     * resumed mid-run cannot come out differently.
     *
     * **It does no file I/O of its own.** SaveGameFile reads and writes;
     * this turns a SaveGame into live state and back. Keeping them apart
     * is what lets a round trip be asserted with no file at all, exactly
     * as saveGameToJson() is split from saveGameFile().
     *
     * A save carries its own GridExtent, which this does not apply: the
     * bounds a run may build within are the run's configuration, and a
     * file quietly widening or narrowing them would let a loaded session
     * hold cells the live one refuses to place. What comes back is the
     * grid, the camera and the plain state.
     */
    class SessionStore final
    {
    public:
        /**
         * @brief Construct the store over the live session.
         * @param world Holds the path, walker and building entities; a
         * restore destroys what is there and creates what was saved.
         * Must outlive this store.
         * @param paths The live path index. Must outlive this store.
         * @param camera The live camera. Must outlive this store.
         * @param state The plain app state: ticks folded, and score.
         * Must outlive this store.
         * @param extent The bounds this run was configured with, which a
         * summary does not carry.
         * @param seed The seed this run was configured with.
         */
        SessionStore(
            World &world,
            PathIndex &paths,
            Camera &camera,
            GameState &state,
            GridExtent extent,
            std::uint64_t seed);

        SessionStore(const SessionStore &) = delete;
        SessionStore(SessionStore &&) = delete;

        SessionStore &operator=(const SessionStore &) = delete;
        SessionStore &operator=(SessionStore &&) = delete;

        /**
         * @brief Take the session as a save file would hold it.
         * @return What is on the grid right now.
         */
        [[nodiscard]] SaveGame take() const;

        /**
         * @brief Put a save back into the session.
         *
         * Everything already on the grid is destroyed rather than added
         * to, since loading is resuming a session and not merging two.
         * Both the destruction and the creation are *staged*, the way
         * every other write to a World is, so they land at the next
         * commit() together -- which is what keeps a load that happens
         * part-way through a tick from being half-visible to whatever
         * runs after it.
         *
         * A save holds no buildings, because the format does not carry
         * them yet, so a restore leaves none. See
         * ISSUES-game-integrate.md.
         *
         * @param save The state to resume from.
         */
        void restore(const SaveGame &save);

    private:
        World &world;
        PathIndex &paths;
        Camera &camera;
        GameState &state;
        GridExtent extent;
        std::uint64_t seed;
    };

} // namespace antwika::game
