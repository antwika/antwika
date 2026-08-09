#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;

    inline constexpr Color kOverlayInk{
        .red = 245, .green = 247, .blue = 252};

    inline constexpr std::uint32_t kTileWidthPerTextScale = 32;

    struct OverlayLabel final
    {
        Point origin;

        std::uint32_t scale = 1;
    };

    /**
     * @brief Places an overlay value in the middle of a tile.
     *
     * @param text The value as it will be written.
     * @param cell The tile the value belongs to.
     * @param camera The camera the tile is seen through.
     * @return Where and how big to write it, or nothing if the text is
     *         wider or taller than the tile.
     */
    [[nodiscard]] std::optional<OverlayLabel> overlayLabelFor(
        std::string_view text, Cell cell, const Camera &camera);

}
