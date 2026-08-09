#pragma once

#include <cstdint>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    inline constexpr std::int32_t kBaseHalfWidth =
        static_cast<std::int32_t>(kIsoTileSize.width) / 2;

    [[nodiscard]] constexpr std::int32_t scaledToZoom(
        std::int32_t value, const Camera &camera) noexcept
    {
        return value * static_cast<std::int32_t>(camera.halfWidth())
            / kBaseHalfWidth;
    }

    [[nodiscard]] constexpr Point blockAnchor(
        Cell origin, Footprint footprint, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(origin, camera);
        const auto cells = footprint.width + footprint.height;

        return Point{
            .x = top.x
                + (footprint.width - footprint.height)
                    * static_cast<std::int32_t>(camera.halfWidth()),
            .y = top.y
                + cells * static_cast<std::int32_t>(camera.halfHeight())};
    }

    [[nodiscard]] constexpr Rect spriteBounds(
        const AtlasSpec &spec,
        Point anchor,
        const Camera &camera) noexcept
    {
        return Rect{
            .origin =
                {.x = anchor.x - scaledToZoom(spec.pivot.x, camera),
                 .y = anchor.y - scaledToZoom(spec.pivot.y, camera)},
            .size = {
                .width = static_cast<std::uint32_t>(
                    scaledToZoom(
                        static_cast<std::int32_t>(spec.spriteSize.width),
                        camera)),
                .height = static_cast<std::uint32_t>(
                    scaledToZoom(
                        static_cast<std::int32_t>(spec.spriteSize.height),
                        camera))}};
    }

    [[nodiscard]] constexpr Rect spriteBounds(
        const AtlasSpecs &specs,
        AtlasKind kind,
        Point anchor,
        const Camera &camera) noexcept
    {
        return spriteBounds(specs.of(kind), anchor, camera);
    }

    [[nodiscard]] constexpr Rect tileSpriteBounds(
        const AtlasSpecs &specs,
        Cell cell,
        const Camera &camera) noexcept
    {
        return spriteBounds(
            specs,
            AtlasKind::OneByOne,
            blockAnchor(cell, Footprint{}, camera),
            camera);
    }

    [[nodiscard]] constexpr Rect buildingSpriteBounds(
        const AtlasSpecs &specs,
        Cell origin,
        BuildingKind kind,
        const Camera &camera) noexcept
    {
        return spriteBounds(
            specs,
            buildingAtlasOf(kind),
            blockAnchor(origin, footprintOf(kind), camera),
            camera);
    }

    static_assert(
        blockAnchor(Cell{.x = 3, .y = 4}, Footprint{}, Camera())
        == Point{
            .x = cellBounds(Cell{.x = 3, .y = 4}, Camera()).origin.x
                + static_cast<std::int32_t>(Camera().halfWidth()),
            .y = cellBounds(Cell{.x = 3, .y = 4}, Camera()).origin.y
                + 2 * static_cast<std::int32_t>(Camera().halfHeight())});

}
