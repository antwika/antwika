#pragma once

#include <cstdint>
#include <optional>

#include "antwika/geometry/GridCell.hpp"
#include "antwika/geometry/Point.hpp"
#include "antwika/geometry/Rect.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    struct Grid final
    {
        Point originPoint{};
        std::uint32_t cell = 0;
        std::uint32_t columns = 0;
        std::uint32_t rows = 0;

        [[nodiscard]] bool operator==(const Grid &other) const = default;
    };

    [[nodiscard]] constexpr std::optional<Grid> gridFit(
        const Rect areaRect,
        const std::uint32_t columns,
        const std::uint32_t rows) noexcept
    {
        if (columns == 0 || rows == 0)
        {
            return std::nullopt;
        }

        const auto byWidth = areaRect.size.width / columns;
        const auto byHeight = areaRect.size.height / rows;
        const auto cell = byWidth < byHeight ? byWidth : byHeight;

        if (cell == 0)
        {
            return std::nullopt;
        }

        const auto usedSize =
            Size{.width = cell * columns, .height = cell * rows};

        return Grid{
            .originPoint = {
                .x = areaRect.originPoint.x
                     + static_cast<std::int32_t>(
                         (areaRect.size.width - usedSize.width) / 2),
                .y = areaRect.originPoint.y
                     + static_cast<std::int32_t>(
                         (areaRect.size.height - usedSize.height) / 2)},
            .cell = cell,
            .columns = columns,
            .rows = rows};
    }

    [[nodiscard]] constexpr std::optional<Grid> gridFitBelow(
        const Size canvasSize,
        const std::uint32_t reservedPixels,
        const std::uint32_t columns,
        const std::uint32_t rows) noexcept
    {
        if (canvasSize.height <= reservedPixels)
        {
            return std::nullopt;
        }

        return gridFit(
            Rect{
                .originPoint = {.x = 0,
                           .y = static_cast<std::int32_t>(reservedPixels)},
                .size = {.width = canvasSize.width,
                         .height = canvasSize.height - reservedPixels}},
            columns,
            rows);
    }

    [[nodiscard]] constexpr std::optional<GridCell> cellAt(
        const Grid &grid, const Point point) noexcept
    {
        if (grid.cell == 0)
        {
            return std::nullopt;
        }

        const auto localX =
            static_cast<std::int64_t>(point.x) - grid.originPoint.x;
        const auto localY =
            static_cast<std::int64_t>(point.y) - grid.originPoint.y;

        if (localX < 0 || localY < 0)
        {
            return std::nullopt;
        }

        const auto column = static_cast<std::uint64_t>(localX) / grid.cell;
        const auto row = static_cast<std::uint64_t>(localY) / grid.cell;

        if (column >= grid.columns || row >= grid.rows)
        {
            return std::nullopt;
        }

        return GridCell{
            .column = static_cast<std::uint32_t>(column),
            .row = static_cast<std::uint32_t>(row)};
    }

    [[nodiscard]] constexpr Rect cellRect(
        const Grid &grid, const GridCell cell) noexcept
    {
        return Rect{
            .originPoint = {
                .x = grid.originPoint.x
                     + static_cast<std::int32_t>(cell.column * grid.cell),
                .y = grid.originPoint.y
                     + static_cast<std::int32_t>(cell.row * grid.cell)},
            .size = {.width = grid.cell, .height = grid.cell}};
    }

}
