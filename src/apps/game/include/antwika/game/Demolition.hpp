#pragma once

#include <antwika/ecs/World.hpp>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/GridExtent.hpp"

namespace antwika::game
{

    using antwika::ecs::World;

    /**
     * @brief Tear one building down and turn its occupants out.
     *
     * **The one statement of what a demolition is**, whoever asked for
     * it: BuildingSystem calls this for a building lost to fire or
     * hunger, and GridSink calls it for one the raze tool was clicked
     * on, so what happens to the people inside cannot depend on why the
     * roof came down.
     *
     * The occupants are spawned in the building's place -- at its
     * origin cell, on the ground the block is about to stop covering --
     * as ordinary migrants walking to the nearest house with room, and
     * failing that to the nearest way off the map, which is exactly
     * where somebody turned out of a shrinking house already goes.
     * A migrant crosses open ground, so no road is needed at the door.
     *
     * The nearest vacancy is asked once and its spare beds are counted:
     * as many leavers as it has room for head there, and the rest make
     * for the gate rather than crowding a doorway that cannot take
     * them. Whoever exceeds the walker limit, or has neither a vacancy
     * nor a gate to walk to, is gone -- the rule every displaced person
     * here already lives under.
     *
     * The building's entry is erased from the index at once and the
     * entity's destruction is staged, so within this tick the index
     * already answers "nothing here" while the World still hands out
     * the last commit -- the same split every placement makes, read
     * backwards.
     *
     * @param world Read for the household; the leavers are created
     * here and the building's destruction is staged here.
     * @param built The building's cells are erased from it.
     * @param entity The building to tear down; must be alive and carry
     * a Building and a Cell.
     * @param extent The bounds the vacancy and gate searches run over.
     */
    void demolish(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent);

} // namespace antwika::game
