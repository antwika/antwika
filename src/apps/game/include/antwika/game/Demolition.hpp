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
     * it: BuildingSystem calls this for a building lost to hunger, and
     * GridSink calls it for one the raze tool was clicked on, so what
     * happens to the people inside cannot depend on why the roof came
     * down. A building lost to *fire* takes ignite() below instead,
     * which shares the people rule and differs about the ground.
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

    /**
     * @brief Set one building alight and turn its occupants out.
     *
     * demolish()'s other ending, and the two share their people rule:
     * the occupants leave as ordinary migrants, a counted few to the
     * nearest vacancy and the rest to the nearest gate, on exactly the
     * terms the header above states.
     *
     * **What differs is the ground.** The Building entity dies here
     * and a Ruin entity stands up in its place, burning, and the block
     * stays in the index -- so nothing can be built where the fire is,
     * a route walks round it, and only the raze tool ever frees the
     * cells. Because the block still stands, the leavers cannot spawn
     * on it: they step out at the lowest free cell round the block's
     * own perimeter, and a building walled in on every side turns its
     * people out to nowhere -- the rule a walled-in house already
     * lives under.
     *
     * @param world Read for the household; the leavers and the ruin
     * are created here and the building's destruction is staged here.
     * @param built Consulted for the escape cell and left holding the
     * block.
     * @param entity The building to set alight; must be alive and
     * carry a Building and a Cell.
     * @param extent The bounds the vacancy and gate searches run over.
     */
    void ignite(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent);

    /**
     * @brief Drop one building to debris and turn its occupants out.
     *
     * The fire's ending without the fire: what collapse risk running
     * out comes to.
     * Identical to ignite() in every rule -- the people leave at the
     * perimeter, the block stays in the index, a Ruin stands up on
     * it -- except that the ruin starts as debris, so no fireman is
     * called and there is nothing to put out.
     *
     * @param world Read for the household; the leavers and the ruin
     * are created here and the building's destruction is staged here.
     * @param built Consulted for the escape cell and left holding the
     * block.
     * @param entity The building to drop; must be alive and carry a
     * Building and a Cell.
     * @param extent The bounds the vacancy and gate searches run over.
     */
    void collapse(
        World &world,
        BuildingIndex &built,
        antwika::ecs::Entity entity,
        GridExtent extent);

} // namespace antwika::game
