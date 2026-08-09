#pragma once

#include <cstdint>
#include <optional>

#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    [[nodiscard]] std::optional<Direction> stepTowards(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent);

    [[nodiscard]] std::optional<std::int64_t> routeCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const PathIndex &paths,
        GridExtent extent);

    [[nodiscard]] std::optional<Direction> stepAcross(
        Cell from,
        Cell goal,
        Footprint footprint,
        const BuildingIndex &built,
        GridExtent extent);

    [[nodiscard]] std::optional<std::int64_t> crossingCost(
        Cell from,
        Cell goal,
        Footprint footprint,
        const BuildingIndex &built,
        GridExtent extent);

}
