#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/game/Camera.hpp"
#include "antwika/game/Cell.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/SceneSnapshot.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;

    /**
     * @brief How many kinds of walker there are.
     *
     * Derived from the last enumerator rather than written out, so a
     * fifth kind cannot be added without a tint for it.
     */
    inline constexpr std::size_t kWalkerKindCount =
        static_cast<std::size_t>(WalkerKind::Architect) + 1;

    /**
     * @brief Get the colour a walker of this kind is modulated by.
     *
     * A tint rather than a tile of its own, because the walker art
     * already spends a slot per facing and what a walker carries is the
     * second thing to tell about it. Every tint is bright, so it shifts
     * the art's hue rather than darkening it.
     *
     * @param kind What the walker is carrying, and for whom.
     * @return The tint to blit that walker's tile with.
     */
    [[nodiscard]] constexpr Color walkerTint(WalkerKind kind) noexcept
    {
        constexpr std::array<Color, kWalkerKindCount> kTints{{
            {.red = 150, .green = 255, .blue = 150, .alpha = 255},
            {.red = 150, .green = 200, .blue = 255, .alpha = 255},
            {.red = 255, .green = 150, .blue = 140, .alpha = 255},
            {.red = 215, .green = 175, .blue = 255, .alpha = 255},
        }};

        return kTints[static_cast<std::size_t>(kind) % kWalkerKindCount];
    }

    /**
     * @brief Get where a building's stock bar stands on screen.
     *
     * Above the cell's **top corner**, which is the point cellToScreen()
     * hands back -- so the bar travels with the same projection every
     * tile goes through, and a pan or a zoom cannot leave it behind.
     * Its size is derived from the camera's half-width for the same
     * reason: a bar of fixed pixels would swamp the grid at the furthest
     * zoom and vanish at the closest.
     *
     * @param cell The cell the building stands on.
     * @param camera Supplies the zoom and the pan.
     * @return The bar's whole extent, filled and empty parts together.
     */
    [[nodiscard]] constexpr Rect stockBarBounds(
        Cell cell, const Camera &camera) noexcept
    {
        const auto top = cellToScreen(cell, camera);
        const auto height = camera.halfWidth();
        const auto width = std::max(2U, camera.halfWidth() / 4U);

        return Rect{
            .origin =
                {.x = top.x - static_cast<std::int32_t>(width / 2),
                 .y = top.y - static_cast<std::int32_t>(height)},
            .size = {.width = width, .height = height}};
    }

    /**
     * @brief Get the filled part of a building's stock bar.
     *
     * Integer arithmetic throughout, and it never divides: a building
     * with no capacity at all reads as empty rather than as a division
     * by zero. Stock above capacity fills the bar rather than
     * overflowing it, since a bar taller than its background would draw
     * over whatever is behind it.
     *
     * The filled part grows from the bottom, so a bar reads the way a
     * gauge does.
     *
     * @param building The building whose stock to show.
     * @param camera Supplies the zoom and the pan.
     * @return The filled extent; its height is zero when nothing is
     * held.
     */
    [[nodiscard]] constexpr Rect stockFillBounds(
        const BuildingView &building, const Camera &camera) noexcept
    {
        const auto bar = stockBarBounds(building.at, camera);
        const auto bottom =
            bar.origin.y + static_cast<std::int32_t>(bar.size.height);

        if (building.capacity <= 0 || building.held <= 0)
        {
            return Rect{
                .origin = {.x = bar.origin.x, .y = bottom},
                .size = {.width = bar.size.width, .height = 0}};
        }

        const auto held = std::min(building.held, building.capacity);
        const auto filled = static_cast<std::uint32_t>(
            (static_cast<std::int64_t>(held) * bar.size.height)
            / building.capacity);

        return Rect{
            .origin =
                {.x = bar.origin.x,
                 .y = bottom - static_cast<std::int32_t>(filled)},
            .size = {.width = bar.size.width, .height = filled}};
    }

    /**
     * @brief Draws a snapshot: the ground, the roads, the buildings and
     * the walkers.
     *
     * Stateless and deterministic on purpose, like apps/life's BoardScene
     * and apps/poker's TableScene. The same snapshot and canvas always
     * produce the same drawing calls in the same order, which is what
     * makes the picture assertable against a mock renderer instead of
     * having to be looked at.
     *
     * Every tile is one blit from one atlas texture, addressed through
     * TileAtlas. The scene draws no shape of its own: the lattice is
     * painted into the ground tile's own edges, so a grid line is a
     * property of the art rather than a line the scene has to place, and
     * a junction is a tile rather than four stubs stepped out by hand.
     *
     * Which road tile a cell shows is decided here, from the snapshot's
     * paths, which arrive in ascending order -- so a neighbour is a
     * binary search rather than a second index the scene would have to be
     * handed and kept in step with. The buildings arrive in that same
     * order and are searched the same way.
     *
     * A stock bar is the one thing here that is not a blit, because its
     * height is a number rather than a picture. It is drawn as two
     * rectangles through the same projection the tiles go through.
     *
     * Two things keep the cost proportional to what is on screen rather
     * than to how big the grid is. Only cells whose diamonds reach the
     * canvas are drawn at all, and a cell is one blit whatever it holds.
     */
    class GridScene final
    {
    public:
        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into.
         * @param snapshot What to draw.
         * @param atlas The texture every tile is blitted from; it must
         * have come from this renderer, and must be the atlas
         * src/apps/game/assets/atlas.png holds, laid out as
         * docs/game-texture-atlas.md describes.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas) const;

    private:
        void drawGround(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot,
            const ITexture &atlas) const;

        void drawStockBars(
            IRenderer &renderer,
            Size canvas,
            const SceneSnapshot &snapshot) const;

        [[nodiscard]] static bool onCanvas(
            Cell cell, Size canvas, const SceneSnapshot &snapshot);
    };

} // namespace antwika::game
