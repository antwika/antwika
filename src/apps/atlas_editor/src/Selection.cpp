#include "antwika/atlas_editor/Selection.hpp"

#include <algorithm>
#include <cstdint>
#include <optional>

namespace antwika::atlas_editor
{

    namespace
    {
        [[nodiscard]] std::int32_t endOf(
            const std::int32_t origin,
            const std::uint32_t extent) noexcept
        {
            return origin + static_cast<std::int32_t>(extent);
        }
    }

    Selection selectionBetween(const Pixel from, const Pixel to) noexcept
    {
        const std::int32_t left = std::min(from.x, to.x);
        const std::int32_t top = std::min(from.y, to.y);
        const std::int32_t right = std::max(from.x, to.x);
        const std::int32_t bottom = std::max(from.y, to.y);

        return Selection{
            .origin = {.x = left, .y = top},
            .size = {
                .width = static_cast<std::uint32_t>(right - left + 1),
                .height = static_cast<std::uint32_t>(bottom - top + 1)}};
    }

    bool contains(
        const Selection &selection, const Pixel pixel) noexcept
    {
        return pixel.x >= selection.origin.x
               && pixel.y >= selection.origin.y
               && pixel.x < endOf(selection.origin.x, selection.size.width)
               && pixel.y
                      < endOf(selection.origin.y, selection.size.height);
    }

    Selection movedBy(
        const Selection &selection,
        const std::int32_t across,
        const std::int32_t down) noexcept
    {
        return Selection{
            .origin =
                {.x = selection.origin.x + across,
                 .y = selection.origin.y + down},
            .size = selection.size};
    }

    std::optional<Selection> clampedTo(
        const Selection &selection, const Size sheet) noexcept
    {
        const std::int32_t left = std::max(selection.origin.x, 0);
        const std::int32_t top = std::max(selection.origin.y, 0);

        const std::int32_t right = std::min(
            endOf(selection.origin.x, selection.size.width),
            static_cast<std::int32_t>(sheet.width));
        const std::int32_t bottom = std::min(
            endOf(selection.origin.y, selection.size.height),
            static_cast<std::int32_t>(sheet.height));

        if (right <= left || bottom <= top)
        {
            return std::nullopt;
        }

        return Selection{
            .origin = {.x = left, .y = top},
            .size = {
                .width = static_cast<std::uint32_t>(right - left),
                .height = static_cast<std::uint32_t>(bottom - top)}};
    }

}
