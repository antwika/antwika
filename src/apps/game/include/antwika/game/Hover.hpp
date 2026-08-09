#pragma once

#include <optional>

#include <antwika/input/PointerHint.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    [[nodiscard]] HoverReadout hoverFor(
        const std::optional<antwika::input::PointerHint> &hint,
        const Camera &camera,
        const SceneSnapshot &snapshot,
        bool coveredByUi);

}
