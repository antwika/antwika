#pragma once

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    /**
     * @brief Check whether a block of cells will take a building.
     *
     * **The one place the rule is stated**, so the ghost somebody sees
     * and the placement they get cannot drift apart.
     * The alternative -- working blocked-ness out inside GridScene from
     * the snapshot's paths and buildings -- states it twice, and two
     * statements of one rule agree only until either is edited.
     *
     * @param origin The minimum-x, minimum-y cell of the block.
     * @param footprint How many cells across and down it covers.
     * @param extent The bounds it must fit inside.
     * @param paths The roads it must not sit on.
     * @param built The buildings it must not sit on.
     * @return True when every covered cell is in bounds, unpaved and
     * unbuilt.
     */
    [[nodiscard]] bool canPlace(
        Cell origin,
        Footprint footprint,
        GridExtent extent,
        const PathIndex &paths,
        const BuildingIndex &built);

    /**
     * @brief Check whether a road may be laid on a cell.
     *
     * A road is a building of one cell as far as this rule goes, so it
     * is the same question asked with a footprint of one -- which is
     * why there is one predicate here rather than two.
     *
     * @param cell The cell to lay on.
     * @param extent The bounds it must be inside.
     * @param paths The roads already laid.
     * @param built The buildings already up.
     * @return True when the cell is in bounds, unpaved and unbuilt.
     */
    [[nodiscard]] bool canPave(
        Cell cell,
        GridExtent extent,
        const PathIndex &paths,
        const BuildingIndex &built);

} // namespace antwika::game
