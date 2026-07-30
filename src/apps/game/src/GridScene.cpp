#include "antwika/game/GridScene.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/TileAtlas.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr Color kSky{.red = 18, .green = 20, .blue = 28};

        // What a stock bar is drawn out of.
        // Dark behind, so the filled part reads at any zoom.
        constexpr Color kStockEmpty{
            .red = 16, .green = 18, .blue = 24, .alpha = 200};
        constexpr Color kStockHeld{
            .red = 96, .green = 200, .blue = 120, .alpha = 255};

        // An opaque white tint leaves a texture exactly as it was.
        // The art already carries every colour the grid has.
        constexpr Color kUntinted{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        [[nodiscard]] bool overlaps(Rect box, Size canvas) noexcept
        {
            const auto right =
                box.origin.x + static_cast<std::int32_t>(box.size.width);
            const auto bottom =
                box.origin.y + static_cast<std::int32_t>(box.size.height);

            return right >= 0 && bottom >= 0
                   && box.origin.x <= static_cast<std::int32_t>(canvas.width)
                   && box.origin.y
                          <= static_cast<std::int32_t>(canvas.height);
        }

        // The paths arrive in ascending order.
        // So asking about one is a search rather than a scan.
        [[nodiscard]] bool paved(
            const std::vector<Cell> &paths, Cell cell) noexcept
        {
            return std::binary_search(paths.begin(), paths.end(), cell);
        }

        [[nodiscard]] std::uint8_t linksAt(
            const std::vector<Cell> &paths, Cell cell) noexcept
        {
            std::uint8_t links = 0;

            for (const auto direction : {
                     Direction::North,
                     Direction::East,
                     Direction::South,
                     Direction::West,
                 })
            {
                if (paved(paths, step(cell, direction)))
                {
                    links = static_cast<std::uint8_t>(
                        links | linkBit(direction));
                }
            }

            return links;
        }
    } // namespace

    bool GridScene::onCanvas(
        Cell cell, Size canvas, const SceneSnapshot &snapshot)
    {
        return overlaps(cellBounds(cell, snapshot.camera), canvas);
    }

    void GridScene::draw(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const ITexture &atlas) const
    {
        renderer.clear(kSky);

        drawGround(renderer, canvas, snapshot, atlas);

        // A road covers the ground tile it is laid on exactly.
        // So it is drawn over one rather than instead of one.
        for (const auto cell : snapshot.paths)
        {
            if (!onCanvas(cell, canvas, snapshot))
            {
                continue;
            }

            renderer.drawTexture(
                atlas,
                roadTile(linksAt(snapshot.paths, cell)),
                cellBounds(cell, snapshot.camera),
                kUntinted);
        }

        // The order is ground, roads, buildings, walkers, stock bars.
        // Ground and roads are flat and cover their own cell exactly.
        // So those two layer rather than overlap.
        //
        // A building is the first thing here that stands up.
        // It therefore needs a back-to-front order, and gets one free.
        // Its art is drawn inside its own cell's bounding box.
        // Two such boxes overlap only for orthogonal neighbours.
        // A neighbour differs by one in exactly one coordinate.
        // So for a neighbour, ascending Cell is ascending screen depth.
        // The order the snapshot arrives in is already back to front.
        // Nothing here has to sort or index anything a second time.
        //
        // Walkers come after, as they did before buildings existed.
        // A walker is then never hidden by what it is standing on.
        // That is also why one beside a building draws over it.
        for (const auto &building : snapshot.buildings)
        {
            if (!onCanvas(building.at, canvas, snapshot))
            {
                continue;
            }

            renderer.drawTexture(
                atlas,
                buildingTile(building.kind),
                cellBounds(building.at, snapshot.camera),
                kUntinted);
        }

        for (const auto &walker : snapshot.walkers)
        {
            if (!onCanvas(walker.at, canvas, snapshot))
            {
                continue;
            }

            renderer.drawTexture(
                atlas,
                walkerTile(walker.facing),
                cellBounds(walker.at, snapshot.camera),
                walkerTint(walker.kind));
        }

        // Last of all, since a bar is a readout rather than scenery.
        // It stands above its cell, outside the box argued about above.
        // So the only order that reads is one hiding no bar at all.
        drawStockBars(renderer, canvas, snapshot);
    }

    void GridScene::drawStockBars(
        IRenderer &renderer, Size canvas, const SceneSnapshot &snapshot) const
    {
        for (const auto &building : snapshot.buildings)
        {
            if (!onCanvas(building.at, canvas, snapshot))
            {
                continue;
            }

            renderer.drawRect(
                stockBarBounds(building.at, snapshot.camera), kStockEmpty);

            const auto filled = stockFillBounds(building, snapshot.camera);

            // An empty bar is its background and nothing else.
            // A zero-height rectangle draws nothing anyway.
            // And a call that draws nothing is one to explain.
            if (filled.size.height > 0)
            {
                renderer.drawRect(filled, kStockHeld);
            }
        }
    }

    void GridScene::drawGround(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const ITexture &atlas) const
    {
        for (std::int32_t y = 0; y < snapshot.extent.height; ++y)
        {
            for (std::int32_t x = 0; x < snapshot.extent.width; ++x)
            {
                const Cell cell{.x = x, .y = y};

                if (!onCanvas(cell, canvas, snapshot))
                {
                    continue;
                }

                renderer.drawTexture(
                    atlas,
                    groundTile(),
                    cellBounds(cell, snapshot.camera),
                    kUntinted);
            }
        }
    }

} // namespace antwika::game
