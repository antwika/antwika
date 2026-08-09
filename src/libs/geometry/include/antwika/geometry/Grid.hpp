#pragma once

#include <cstdint>
#include <optional>

#include "antwika/geometry/Point.hpp"
#include "antwika/geometry/Rect.hpp"
#include "antwika/geometry/Size.hpp"

namespace antwika::geometry
{

    struct GridCell final
    {
        std::uint32_t column = 0;
        std::uint32_t row = 0;

        [[nodiscard]] bool operator==(const GridCell &other) const
            = default;
    };

    struct Grid final
    {
        Point origin{};
        std::uint32_t cell = 0;
        std::uint32_t columns = 0;
        std::uint32_t rows = 0;

        [[nodiscard]] bool operator==(const Grid &other) const = default;
    };

    [[nodiscard]] constexpr std::optional<Grid> gridFit(
        const Rect area,
        const std::uint32_t columns,
        const std::uint32_t rows) noexcept
    {
        if (columns == 0 || rows == 0)
        {
            return std::nullopt;
        }

        const auto byWidth = area.size.width / columns;
        const auto byHeight = area.size.height / rows;
        const auto cell = byWidth < byHeight ? byWidth : byHeight;

        if (cell == 0)
        {
            return std::nullopt;
        }

        const auto used =
            Size{.width = cell * columns, .height = cell * rows};

        return Grid{
            .origin = {
                .x = area.origin.x
                     + static_cast<std::int32_t>(
                         (area.size.width - used.width) / 2),
                .y = area.origin.y
                     + static_cast<std::int32_t>(
                         (area.size.height - used.height) / 2)},
            .cell = cell,
            .columns = columns,
            .rows = rows};
    }

    [[nodiscard]] constexpr std::optional<Grid> gridFitBelow(
        const Size canvas,
        const std::uint32_t reserved,
        const std::uint32_t columns,
        const std::uint32_t rows) noexcept
    {
        if (canvas.height <= reserved)
        {
            return std::nullopt;
        }

        return gridFit(
            Rect{
                .origin = {.x = 0,
                           .y = static_cast<std::int32_t>(reserved)},
                .size = {.width = canvas.width,
                         .height = canvas.height - reserved}},
            columns,
            rows);
    }

    [[nodiscard]] constexpr std::optional<GridCell> cellAt(
        const Grid &grid, const Point at) noexcept
    {
        if (grid.cell == 0)
        {
            return std::nullopt;
        }

        const auto localX =
            static_cast<std::int64_t>(at.x) - grid.origin.x;
        const auto localY =
            static_cast<std::int64_t>(at.y) - grid.origin.y;

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
            .origin = {
                .x = grid.origin.x
                     + static_cast<std::int32_t>(cell.column * grid.cell),
                .y = grid.origin.y
                     + static_cast<std::int32_t>(cell.row * grid.cell)},
            .size = {.width = grid.cell, .height = grid.cell}};
    }

}
