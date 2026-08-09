#pragma once

#include <optional>

#include <antwika/input/PointerHint.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/BuildingIndex.hpp"
#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/GridExtent.hpp"
#include "antwika/game/PathIndex.hpp"

namespace antwika::game
{

    struct BuildGhost final
    {
        Cell at{};
        BuildTool tool = BuildTool::Road;

        bool visible = false;

        bool valid = false;

        [[nodiscard]] bool operator==(
            const BuildGhost &other) const = default;
    };

    [[nodiscard]] BuildGhost ghostFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        GridExtent extent,
        std::optional<BuildTool> tool,
        bool coveredByUi,
        const PathIndex &paths,
        const BuildingIndex &built);

}
