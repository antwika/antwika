#include "antwika/game/GridScene.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/TileAtlas.hpp"
#include "antwika/game/WalkerMotion.hpp"

namespace antwika::game
{

    using antwika::gfx::Color;
    using antwika::gfx::Rect;

    namespace
    {
        constexpr Color kSky{.red = 18, .green = 20, .blue = 28};

        // An opaque white tint leaves a texture exactly as it was.
        // The art already carries every colour the grid has.
        constexpr Color kUntinted{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        // The same art, mostly transparent.
        // A placeholder then reads as the thing it stands for.
        // drawTexture() modulates alpha, so no second tile is needed.
        constexpr Color kGhostly{
            .red = 255, .green = 255, .blue = 255, .alpha = 110};

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
        const ITexture &atlas,
        Progress subTick) const
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

        // Over the road it may stand beside, under the walkers.
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

        // Last, so a walker is never hidden by what it is standing on.
        for (const auto &walker : snapshot.walkers)
        {
            // Culled on where it is drawn, not on the cell it is on.
            // Between two ticks those are not the same box.
            // And a walker halfway off the edge is still half on it.
            const auto bounds = walkerBounds(walker, snapshot.camera, subTick);

            if (!overlaps(bounds, canvas))
            {
                continue;
            }

            renderer.drawTexture(
                atlas, walkerTile(walker.facing), bounds, kUntinted);
        }

        drawGhost(renderer, canvas, snapshot, atlas);
    }

    void GridScene::drawGhost(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const ITexture &atlas) const
    {
        const auto &ghost = snapshot.ghost;

        if (!ghost.visible || !onCanvas(ghost.at, canvas, snapshot))
        {
            return;
        }

        // A road ghost shows the junction it would become.
        // Worked out the same way a laid one is.
        // So what is previewed is what is placed.
        renderer.drawTexture(
            atlas,
            toolTile(ghost.tool, linksAt(snapshot.paths, ghost.at)),
            cellBounds(ghost.at, snapshot.camera),
            kGhostly);
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
