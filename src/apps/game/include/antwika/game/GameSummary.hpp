#pragma once

#include <vector>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    /**
     * @brief Everything a run amounted to, as one comparable value.
     *
     * World holds live entities and cannot be copied or compared, so this
     * is the read-only copy taken from it instead -- the role apps/life's
     * Board plays there.
     *
     * Comparable on purpose: it is what lets a live run and its replay be
     * asserted equal, which is the claim the whole design rests on. A
     * hash would agree for the wrong reason if both runs did nothing.
     *
     * The camera is in here because it is simulation state, not because a
     * summary needs to know where the view was -- see Camera.
     */
    struct GameSummary
    {
        GameState state;
        std::vector<Cell> paths;
        std::vector<WalkerView> walkers;
        std::vector<BuildingView> buildings;
        Camera camera;

        /**
         * @brief Compare two summaries.
         * @param other The summary to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const GameSummary &other) const = default;
    };

} // namespace antwika::game
