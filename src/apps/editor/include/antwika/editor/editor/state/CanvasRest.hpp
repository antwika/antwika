#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include "antwika/editor/ui/ColorPicker.hpp"

namespace antwika::editor
{

    struct CanvasRest final
    {
        std::optional<geometry::GridCell> tileCell;

        std::optional<std::size_t> face;

        std::uint32_t sinceTick = 0;
    };

}
