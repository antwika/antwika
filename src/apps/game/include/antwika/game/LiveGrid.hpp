#pragma once

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief The one grid a session builds on, whichever city owns it.
     *
     * A struct of borrowed references rather than a parameter list, for
     * the reason RenderSetup gives: these four are swapped together or
     * not at all, and a positional list of them is a row of references
     * distinguishable only by where they sit.
     *
     * Naming them together is also what makes "the live grid" a thing
     * the code can say. A city is opened by putting its contents into
     * these and closed by taking them back out -- see WorldMapState --
     * so every collaborator that builds, walks or draws the grid holds
     * one reference rather than resolving a city index on every call.
     *
     * Every member is borrowed and must outlive whatever holds this.
     */
    struct LiveGrid
    {
        /** @brief Holds the path, walker and building entities. */
        World &world;

        /** @brief Which cells have a road on them. */
        PathIndex &paths;

        /**
         * @brief Which cells have a building on them.
         *
         * Derived from the buildings rather than kept per city, so it
         * cannot be swapped out of step with the world it describes --
         * restoreCityGrid() rebuilds it from what it puts down.
         */
        BuildingIndex &built;

        /** @brief Where the grid is looked at from, and how closely. */
        Camera &camera;
    };

} // namespace antwika::game
