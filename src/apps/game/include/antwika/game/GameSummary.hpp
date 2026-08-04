#pragma once

#include <string>
#include <vector>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/CityRatings.hpp"
#include "antwika/game/GameState.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/KeyboardLayout.hpp"
#include "antwika/game/SceneSnapshot.hpp"
#include "antwika/game/Toolbar.hpp"

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

        /**
         * @brief Every building placed, in the order they were placed.
         *
         * Here for the same reason the walkers are: a run that built
         * something should be able to say so, and a live run and its
         * replay disagreeing about it should fail the comparison.
         */
        std::vector<BuildingView> buildings;

        /**
         * @brief Every fire and heap of debris, in the world's order.
         *
         * Here so a divergence in what burnt fails the comparison
         * directly: a building that catches fire leaves the buildings
         * list, so without this a live run and its replay could
         * disagree about a whole district having burned and compare
         * equal on everything but the walkers it displaced.
         */
        std::vector<RuinView> ruins;

        Camera camera;

        /**
         * @brief How the city was doing when the run ended.
         *
         * **Here so that a divergence in the city's people fails the
         * replay comparison directly.** Population and employment are
         * sums over state no other member of this summary carries: a
         * house's occupancy is not in BuildingView and a workplace's
         * share of the workforce is not either, so without this a live
         * run and its replay could disagree about both and still be
         * equal.
         *
         * Not persisted, and nothing here is: every member is a sum over
         * what a save already holds -- see CityRatings.
         */
        CityRatings ratings;

        /**
         * @brief Every line the console held when the run ended.
         *
         * **Here so that a divergence in the console fails the replay
         * comparison directly.** The history is simulation state --
         * what a dump carries and what the open console lists -- so a
         * live run and its replay disagreeing about it is the silent
         * divergence this summary exists to catch.
         *
         * A run that never opened the console ends with it empty,
         * which is what every summary held before this member existed.
         */
        std::vector<std::string> console;

        /**
         * @brief Which board the run's typing was read off at the end.
         *
         * **Here so that a divergence in the layout fails the replay
         * comparison directly**, on the bindings' exact argument: the
         * layout decides what every later key press types, so two
         * runs disagreeing about it could type two different commands
         * from one recording.
         *
         * A run that was never told otherwise ends on
         * kDefaultKeyboardLayout.
         */
        KeyboardLayout keyboard{kDefaultKeyboardLayout};

        /**
         * @brief Which key asked for what, when the run ended.
         *
         * **Here so that a divergence in the bindings fails the replay
         * comparison directly.** A binding decides what every later key
         * press means, so a live run and its replay disagreeing about
         * one is exactly the silent divergence the whole arrangement
         * exists to prevent -- and without this the two could disagree
         * about a key nobody happened to press afterwards and still
         * compare equal.
         *
         * A run that was never told otherwise ends on kDefaultBindings,
         * which is what every summary held before this member existed.
         */
        KeyBindings bindings;

        /**
         * @brief Compare two summaries.
         * @param other The summary to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const GameSummary &other) const = default;
    };

} // namespace antwika::game
