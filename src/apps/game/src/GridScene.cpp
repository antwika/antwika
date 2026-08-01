#include "antwika/game/GridScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/FootprintOutline.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/ResourceBar.hpp"
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

        // The same tile reddened, for a block that will not go here.
        // A refusal shown is a refusal somebody can act on.
        // A preview that vanishes leaves them guessing what blocked it.
        constexpr Color kBlocked{
            .red = 255, .green = 90, .blue = 90, .alpha = 110};

        // The border round the block, at full strength.
        // The tile inside it is faint on purpose, being a placeholder.
        // An edge as faint would be the one thing here nobody could see.
        constexpr Color kGhostEdge{
            .red = 255, .green = 255, .blue = 255, .alpha = 220};

        // Reddened for the same reason the tile inside it is.
        constexpr Color kBlockedEdge{
            .red = 255, .green = 90, .blue = 90, .alpha = 220};

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

        // The track always, the fill only when there is any of it.
        // A rectangle of no height is a drawing call that draws nothing.
        void paintBars(
            IRenderer &renderer, const std::vector<ResourceBar> &bars)
        {
            for (const auto &bar : bars)
            {
                renderer.drawRect(bar.track, kBarTrack);

                if (bar.fill.size.height > 0)
                {
                    renderer.drawRect(
                        bar.fill, resourceColour(bar.resource));
                }
            }
        }
    } // namespace

    bool GridScene::onCanvas(
        Cell cell, Size canvas, const SceneSnapshot &snapshot)
    {
        return overlaps(cellBounds(cell, snapshot.camera), canvas);
    }

    GridScene::GridScene(const Translator &translator)
        : translator(translator)
    {
    }

    void GridScene::draw(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const ITexture &atlas,
        Progress subTick) const
    {
        renderer.clear(kSky);

        // A paused walker is drawn at its step's own phase and no more.
        // The whole ticks of a step stop when WalkerSystem does.
        // The frames between two ticks do not stop with them.
        // So a frozen walker would otherwise slide and snap back.
        // Once per tick, for as long as the run stayed held.
        // Decided here rather than by whoever passes the sub-tick.
        // A snapshot then draws the same picture wherever it is drawn.
        const auto phase = snapshot.paused ? Progress() : subTick;

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
        // Drawn from the whole block's box rather than one cell's.
        // Culled on that box too.
        // A block reaches the canvas from further off than one cell.
        for (const auto &building : snapshot.buildings)
        {
            const auto bounds = footprintBounds(
                building.at, footprintOf(building.kind), snapshot.camera);

            if (!overlaps(bounds, canvas))
            {
                continue;
            }

            renderer.drawTexture(
                atlas, buildingTile(building.kind), bounds, kUntinted);
        }

        // Last, so a walker is never hidden by what it is standing on.
        for (const auto &walker : snapshot.walkers)
        {
            // Culled on where it is drawn, not on the cell it is on.
            // Between two ticks those are not the same box.
            // And a walker halfway off the edge is still half on it.
            const auto bounds = walkerBounds(walker, snapshot.camera, phase);

            if (!overlaps(bounds, canvas))
            {
                continue;
            }

            renderer.drawTexture(
                atlas, walkerTile(walker.facing), bounds, kUntinted);
        }

        // After every sprite.
        // A gauge is then never hidden by what stands in front of it.
        // Handed the same phase, so a bar cannot drift off its walker.
        drawBars(renderer, canvas, snapshot, phase);

        // Before the ghost, which is one cell and sits on top of it.
        drawPlan(renderer, canvas, snapshot, atlas);

        drawGhost(renderer, canvas, snapshot, atlas);

        // Last of all, since it is what somebody is reading.
        drawReadout(renderer, canvas, snapshot);
    }

    void GridScene::drawBars(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        Progress subTick) const
    {
        // Culled on the sprite's own box rather than the bar's.
        // A gauge is drawn exactly when what it belongs to is.
        for (const auto &building : snapshot.buildings)
        {
            const auto bounds = footprintBounds(
                building.at, footprintOf(building.kind), snapshot.camera);

            if (!overlaps(bounds, canvas))
            {
                continue;
            }

            paintBars(renderer, buildingBars(building, snapshot.camera));
        }

        for (const auto &walker : snapshot.walkers)
        {
            const auto bounds =
                walkerBounds(walker, snapshot.camera, subTick);

            if (!overlaps(bounds, canvas))
            {
                continue;
            }

            paintBars(
                renderer, walkerBars(walker, snapshot.camera, subTick));
        }
    }

    void GridScene::drawReadout(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot) const
    {
        const auto panel =
            readoutPanel(snapshot.hover, canvas, translator);

        if (panel.lines.empty())
        {
            return;
        }

        renderer.drawRect(panel.box, kReadoutBackdrop);

        for (const auto &line : panel.lines)
        {
            renderer.drawText(
                line.origin, line.text, kReadoutTextScale, line.colour);
        }
    }

    void GridScene::drawPlan(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const ITexture &atlas) const
    {
        // A refused run is reddened rather than hidden.
        // Exactly as a refused block is.
        // The cells it names are the two somebody asked for.
        // So what is shown is the gesture being turned down.
        const auto tint = snapshot.plan.valid ? kGhostly : kBlocked;

        for (const auto cell : snapshot.plan.cells)
        {
            const auto bounds = cellBounds(cell, snapshot.camera);

            if (!overlaps(bounds, canvas))
            {
                continue;
            }

            // The junction each would become.
            // Worked out the same way a laid one is.
            // So what is previewed is what is placed.
            renderer.drawTexture(
                atlas,
                roadTile(linksAt(snapshot.paths, cell)),
                bounds,
                tint);
        }
    }

    void GridScene::drawGhost(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const ITexture &atlas) const
    {
        const auto &ghost = snapshot.ghost;

        if (!ghost.visible)
        {
            return;
        }

        const auto kind = buildingKindOf(ghost.tool);
        const auto footprint =
            kind.has_value() ? footprintOf(*kind) : Footprint{};

        const auto bounds =
            footprintBounds(ghost.at, footprint, snapshot.camera);

        if (!overlaps(bounds, canvas))
        {
            return;
        }

        // A road ghost shows the junction it would become.
        // Worked out the same way a laid one is.
        // So what is previewed is what is placed.
        renderer.drawTexture(
            atlas,
            toolTile(ghost.tool, linksAt(snapshot.paths, ghost.at)),
            bounds,
            ghost.valid ? kGhostly : kBlocked);

        // A border round exactly the cells the click will take.
        // A faint tile says roughly where; an edge says precisely what.
        // Traced round the very box the tile above was blitted into.
        // So a preview and its border cannot show two extents.
        // Four lines rather than four fills, the edges being diagonal.
        // drawRect() takes an upright box, so it cannot draw one.
        // Which is why ui's focus ring is four fills and this is not.
        // drawLine() exists to step diagonal shapes out of.
        // Where its middle pixels land is the backend's business.
        // Nothing reads a line back, so no replay can hear about it.
        const auto corners =
            footprintOutline(ghost.at, footprint, snapshot.camera);
        const auto edge = ghost.valid ? kGhostEdge : kBlockedEdge;

        for (std::size_t corner = 0; corner < corners.size(); ++corner)
        {
            renderer.drawLine(
                corners[corner],
                corners[(corner + 1) % corners.size()],
                edge);
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
