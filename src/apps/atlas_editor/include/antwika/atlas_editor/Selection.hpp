#pragma once

#include <cstdint>
#include <optional>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/Pixel.hpp"

namespace antwika::atlas_editor
{

    using antwika::gfx::Size;

    struct Selection final
    {
        Pixel origin{};

        Size size{};

        [[nodiscard]] bool operator==(const Selection &other) const =
            default;
    };

    [[nodiscard]] Selection selectionBetween(Pixel from, Pixel to) noexcept;

    [[nodiscard]] bool contains(
        const Selection &selection, Pixel pixel) noexcept;

    [[nodiscard]] Selection movedBy(
        const Selection &selection,
        std::int32_t across,
        std::int32_t down) noexcept;

    [[nodiscard]] std::optional<Selection> clampedTo(
        const Selection &selection, Size sheet) noexcept;

}
