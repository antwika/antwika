#pragma once

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/BuildingKind.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    [[nodiscard]] bool canPlace(
        Cell origin,
        Footprint footprint,
        GridExtent extent,
        const PathIndex &paths,
        const BuildingIndex &built);

    [[nodiscard]] bool canPave(
        Cell cell,
        GridExtent extent,
        const PathIndex &paths,
        const BuildingIndex &built);

}
