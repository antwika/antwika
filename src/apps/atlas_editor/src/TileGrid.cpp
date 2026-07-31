#include "antwika/atlas_editor/TileGrid.hpp"

#include <cstdint>
#include <optional>

namespace antwika::atlas_editor
{

    namespace
    {
        // Whole divisions only, and no division by zero.
        // A grid with no extent divides an image into nothing.
        // That is what a caller asking for its columns is told.
        [[nodiscard]] std::uint32_t countOf(
            const std::uint32_t extent, const std::uint32_t step) noexcept
        {
            if (step == 0)
            {
                return 0;
            }

            return extent / step;
        }
    } // namespace

    std::uint32_t columnsIn(const TileGrid grid, const Size image) noexcept
    {
        return countOf(image.width, grid.width);
    }

    std::uint32_t rowsIn(const TileGrid grid, const Size image) noexcept
    {
        return countOf(image.height, grid.height);
    }

    std::optional<std::uint32_t> slotAt(
        const TileGrid grid, const Size image, const Pixel pixel) noexcept
    {
        const std::uint32_t columns = columnsIn(grid, image);
        const std::uint32_t rows = rowsIn(grid, image);

        if (columns == 0 || rows == 0 || pixel.x < 0 || pixel.y < 0)
        {
            return std::nullopt;
        }

        const auto column =
            static_cast<std::uint32_t>(pixel.x) / grid.width;
        const auto row = static_cast<std::uint32_t>(pixel.y) / grid.height;

        if (column >= columns || row >= rows)
        {
            return std::nullopt;
        }

        return row * columns + column;
    }

} // namespace antwika::atlas_editor
