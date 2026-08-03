#include "antwika/game/GridScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/FootprintOutline.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/ReadoutPanel.hpp"
#include "antwika/game/SpriteBounds.hpp"
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
        // drawTexture() modulates alpha, so no second sprite is needed.
        constexpr Color kGhostly{
            .red = 255, .green = 255, .blue = 255, .alpha = 110};

        // The same sprite reddened, for a block that will not go here.
        // A refusal shown is a refusal somebody can act on.
        // A preview that vanishes leaves them guessing what blocked it.
        constexpr Color kBlocked{
            .red = 255, .green = 90, .blue = 90, .alpha = 110};

        // The border round the block, at full strength.
        // The sprite inside it is faint on purpose, being a preview.
        // An edge as faint would be the one thing here nobody could see.
        constexpr Color kGhostEdge{
            .red = 255, .green = 255, .blue = 255, .alpha = 220};

        // Reddened for the same reason the sprite inside it is.
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

        // The junctions a run of road would come out as, in its order.
        // Worked out against the roads and the rest of the run.
        // Which is the whole of why it is not four calls to linksAt().
        // A planned cell's neighbours are mostly other planned cells.
        // The roads already there answer "no arms" for every one.
        // So a connected route came out as a string of loose stubs.
        // Which is nothing like what its release lays.
        // The run is short and unsorted, so membership is a scan.
        // Over at most four candidates a cell, rather than a search.
        [[nodiscard]] std::vector<std::uint8_t> planLinks(
            const std::vector<Cell> &paths, const std::vector<Cell> &plan)
        {
            std::vector<std::uint8_t> links(plan.size(), 0);

            for (std::size_t index = 0; index < plan.size(); ++index)
            {
                for (const auto direction : {
                         Direction::North,
                         Direction::East,
                         Direction::South,
                         Direction::West,
                     })
                {
                    const auto beside = step(plan[index], direction);

                    if (paved(paths, beside)
                        || std::find(plan.begin(), plan.end(), beside)
                               != plan.end())
                    {
                        links[index] = static_cast<std::uint8_t>(
                            links[index] | linkBit(direction));
                    }
                }
            }

            return links;
            // The excluded line is the local vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        /** @brief One block to paint: a building or a ruin. */
        struct BlockArt
        {
            /** @brief The minimum-x, minimum-y cell of its block. */
            Cell at;

            /** @brief What names its footprint and its sheet. */
            BuildingKind kind = BuildingKind::House;

            /** @brief The sprite to blit, in that sheet's pixels. */
            Rect tile;
        };

        // The buildings and the ruins as one back-to-front stream.
        // A ruin is a block exactly as a building is.
        // So the terrain pass paints the two through one merge.
        // A second loop would restate the paint-order rule.
        // Both inputs arrive sorted on the flush key.
        // So this is a merge rather than a sort.
        [[nodiscard]] std::vector<BlockArt> blocksOf(
            const SceneSnapshot &snapshot)
        {
            std::vector<BlockArt> blocks;
            blocks.reserve(
                snapshot.buildings.size() + snapshot.ruins.size());

            const auto deeper = [](Cell left, Cell right)
            {
                const auto depth = left.x + left.y;
                const auto other = right.x + right.y;

                return depth != other ? depth < other : left.x < right.x;
            };

            std::size_t building = 0;
            std::size_t ruin = 0;

            while (building < snapshot.buildings.size()
                   || ruin < snapshot.ruins.size())
            {
                const auto takeBuilding = ruin >= snapshot.ruins.size()
                    || (building < snapshot.buildings.size()
                        && deeper(
                            snapshot.buildings[building].at,
                            snapshot.ruins[ruin].at));

                if (takeBuilding)
                {
                    const auto &sprite = snapshot.buildings[building];
                    blocks.push_back(BlockArt{
                        .at = sprite.at,
                        .kind = sprite.kind,
                        .tile = buildingTile(sprite.kind)});
                    ++building;
                }
                else
                {
                    const auto &sprite = snapshot.ruins[ruin];
                    blocks.push_back(BlockArt{
                        .at = sprite.at,
                        .kind = sprite.kind,
                        .tile = ruinTile(sprite.state, sprite.kind)});
                    ++ruin;
                }
            }

            return blocks;
            // The excluded line is the local vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        // Every cell the snapshot's blocks stand on, ascending.
        // The terrain pass skips these rather than painting under art.
        // A block's art owns its whole footprint anyway.
        [[nodiscard]] std::vector<Cell> coveredCells(
            const std::vector<BlockArt> &blocks)
        {
            std::vector<Cell> covered;

            for (const auto &block : blocks)
            {
                const auto footprint = footprintOf(block.kind);

                for (std::int32_t dy = 0; dy < footprint.height; ++dy)
                {
                    for (std::int32_t dx = 0; dx < footprint.width; ++dx)
                    {
                        covered.push_back(Cell{
                            .x = block.at.x + dx,
                            .y = block.at.y + dy});
                    }
                }
            }

            std::sort(covered.begin(), covered.end());

            return covered;
            // The excluded line is the local vector's unwind destructor.
            // Nothing between its construction and the return throws.
        } // GCOVR_EXCL_LINE

        /** @brief The flank cells to paint ahead of their block. */
        struct PulledFlanks
        {
            /** @brief The cells, ascending, for the skip test. */
            std::vector<Cell> cells;

            /** @brief Origin diagonal and cell, in flush order. */
            std::vector<std::pair<std::int32_t, Cell>> byDiagonal;
        };

        // The uncovered flank neighbours deeper than their block.
        // West and north cells mostly share the origin diagonal.
        // A wide block's corner ones land a diagonal past it.
        // Painted there, after the flush, they stamp over the art.
        [[nodiscard]] PulledFlanks pulledFlanks(
            const std::vector<BlockArt> &blocks,
            const std::vector<Cell> &covered)
        {
            PulledFlanks pulls;

            for (const auto &building : blocks)
            {
                const auto footprint = footprintOf(building.kind);
                const auto origin = building.at.x + building.at.y;

                const auto consider = [&](const Cell cell)
                {
                    // Refused four ways, each for its own reason.
                    // Off the grid, or behind the flush anyway.
                    // Owned by some block's art, or pulled already.
                    // Corner-to-corner blocks share one flank cell.
                    if (cell.x < 0 || cell.y < 0
                        || cell.x + cell.y <= origin
                        || std::binary_search(
                            covered.begin(), covered.end(), cell)
                        || std::find(
                               pulls.cells.begin(), pulls.cells.end(),
                               cell)
                               != pulls.cells.end())
                    {
                        return;
                    }

                    pulls.cells.push_back(cell);
                    pulls.byDiagonal.emplace_back(origin, cell);
                };

                for (std::int32_t dy = 0; dy < footprint.height; ++dy)
                {
                    consider(Cell{
                        .x = building.at.x - 1,
                        .y = building.at.y + dy});
                }

                for (std::int32_t dx = 0; dx < footprint.width; ++dx)
                {
                    consider(Cell{
                        .x = building.at.x + dx,
                        .y = building.at.y - 1});
                }
            }

            // The buildings arrive sorted on their flush key.
            // So byDiagonal is already in flush order.
            // The cells sort apart, for the terrain pass's skip test.
            std::sort(pulls.cells.begin(), pulls.cells.end());

            return pulls;
            // The excluded line is the locals' unwind destructor.
            // Nothing between their construction and the return throws.
        } // GCOVR_EXCL_LINE
    } // namespace

    bool GridScene::onCanvas(
        Cell cell, Size canvas, const SceneSnapshot &snapshot)
    {
        return overlaps(tileSpriteBounds(cell, snapshot.camera), canvas);
    }

    GridScene::GridScene(const Translator &translator)
        : translator(translator)
    {
    }

    void GridScene::draw(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases,
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

        drawTerrain(renderer, canvas, snapshot, atlases);

        // Over the ground and the blocks, under the walkers.
        // A walker is a thing in the city rather than a fact about it.
        // So an overlay reads as painted on rather than over the top.
        drawOverlay(renderer, canvas, snapshot, atlases);

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

            // The frame reads the slide's own pause-adjusted fraction.
            // So a held walker's legs freeze with it.
            renderer.drawTexture(
                atlases.oneByOne,
                walkerTile(walker.facing, walkerFrame(walker, phase)),
                bounds,
                kUntinted);
        }

        // Before the ghost, which is one cell and sits on top of it.
        drawPlan(renderer, canvas, snapshot, atlases);

        drawGhost(renderer, canvas, snapshot, atlases);

        // Last of all, since it is what somebody is reading.
        drawReadout(renderer, canvas, snapshot);
    }

    void GridScene::drawTerrain(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases) const
    {
        const auto blocks = blocksOf(snapshot);
        const auto covered = coveredCells(blocks);
        std::size_t next = 0;

        const auto drawBlock = [&](const BlockArt &block)
        {
            const auto bounds = buildingSpriteBounds(
                block.at, block.kind, snapshot.camera);

            if (!overlaps(bounds, canvas))
            {
                return;
            }

            // The tile came along with the block.
            // So a ruin's art takes the very call a building's takes.
            renderer.drawTexture(
                atlases.of(buildingAtlasOf(block.kind)),
                block.tile,
                bounds,
                kUntinted);
        };

        const auto drawGround = [&](const Cell cell)
        {
            if (!onCanvas(cell, canvas, snapshot))
            {
                return;
            }

            const auto bounds = tileSpriteBounds(cell, snapshot.camera);

            renderer.drawTexture(
                atlases.oneByOne, groundTile(), bounds, kUntinted);

            // A road covers the ground sprite it is laid on exactly.
            // So it is drawn over one rather than instead of one.
            // Its edge pixels then blend into ground, not into sky.
            if (paved(snapshot.paths, cell))
            {
                renderer.drawTexture(
                    atlases.oneByOne,
                    roadTile(linksAt(snapshot.paths, cell)),
                    bounds,
                    kUntinted);
            }
        };

        // The flank cells a wide block's art must never be under.
        // A block flushes with its origin diagonal's terrain.
        // A 3x3's flank corners lie one diagonal deeper than that.
        // Painted on their own diagonal, they stamp over the art.
        // So each is pulled into its block's own terrain phase.
        // A 2x2 has no such cell: its flanks share the origin's.
        const auto pulls = pulledFlanks(blocks, covered);
        std::size_t drawn = 0;

        // One diagonal is one screen depth, walked back to front.
        // The buildings arrive sorted on exactly this key.
        // So each is painted with the diagonal its block starts on.
        // What is south or east of it is then painted after it.
        // Which is what lets its skirt sit under the cells in front.
        const auto diagonals =
            snapshot.extent.width + snapshot.extent.height - 1;

        for (std::int32_t diagonal = 0; diagonal < diagonals; ++diagonal)
        {
            const auto firstX = std::max(
                std::int32_t{0}, diagonal - snapshot.extent.height + 1);
            const auto lastX =
                std::min(diagonal, snapshot.extent.width - 1);

            for (std::int32_t x = firstX; x <= lastX; ++x)
            {
                const Cell cell{.x = x, .y = diagonal - x};

                // A building's art owns the whole block it stands on.
                if (std::binary_search(
                        covered.begin(), covered.end(), cell))
                {
                    continue;
                }

                // A pulled flank cell was painted diagonals ago.
                if (std::binary_search(
                        pulls.cells.begin(), pulls.cells.end(), cell))
                {
                    continue;
                }

                drawGround(cell);
            }

            // The deeper flank cells this diagonal's blocks reach.
            // Behind the art, so ahead of the flush just below.
            while (drawn < pulls.byDiagonal.size()
                   && pulls.byDiagonal[drawn].first <= diagonal)
            {
                drawGround(pulls.byDiagonal[drawn].second);
                ++drawn;
            }

            while (next < blocks.size()
                   && blocks[next].at.x + blocks[next].at.y <= diagonal)
            {
                drawBlock(blocks[next]);
                ++next;
            }
        }

        // A snapshot is drawn whole even where the extent ends.
        // Nothing places a block outside it.
        // A scene still draws what it is handed rather than judging it.
        while (next < blocks.size())
        {
            drawBlock(blocks[next]);
            ++next;
        }
    }

    void GridScene::drawOverlay(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases) const
    {
        // The city itself, with nothing painted over it.
        // Asked of the view rather than of the field being empty.
        // A city nothing has reached yet has an empty field too.
        // And an all-dark map is exactly what it should look like.
        if (snapshot.view == MapView::Normal)
        {
            return;
        }

        const auto ink = overlayColour(snapshot.view);

        // The whole grid rather than only the cells with a value.
        // A district nothing reaches is what somebody looks for here.
        // So it has to be visibly darker rather than simply unpainted.
        for (std::int32_t y = 0; y < snapshot.extent.height; ++y)
        {
            for (std::int32_t x = 0; x < snapshot.extent.width; ++x)
            {
                const Cell cell{.x = x, .y = y};

                if (!onCanvas(cell, canvas, snapshot))
                {
                    continue;
                }

                const auto bounds =
                    tileSpriteBounds(cell, snapshot.camera);

                // The ground sprite, tinted, rather than a rectangle.
                // A cell is a diamond and drawRect() takes a box.
                // Which is the same reason the ghost's edge is lines.
                renderer.drawTexture(
                    atlases.oneByOne, groundTile(), bounds, kOverlayScrim);

                const auto found = snapshot.overlay.find(cell);

                if (found == snapshot.overlay.end())
                {
                    continue;
                }

                // Faintest at the bottom of the scale, never invisible.
                // Or the bottom of it would read as nothing at all.
                const auto strength = kOverlayFaintest
                    + (255 - kOverlayFaintest) * found->second / 100;

                renderer.drawTexture(
                    atlases.oneByOne,
                    groundTile(),
                    bounds,
                    Color{
                        .red = ink.red,
                        .green = ink.green,
                        .blue = ink.blue,
                        .alpha =
                            static_cast<std::uint8_t>(strength)});
            }
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
        const AtlasTextures &atlases) const
    {
        // A refused run is reddened rather than hidden.
        // Exactly as a refused block is.
        // The cells it names are the two somebody asked for.
        // So what is shown is the gesture being turned down.
        const auto tint = snapshot.plan.valid ? kGhostly : kBlocked;

        // The junction each cell becomes once the whole run is laid.
        // Rather than the one it would become on its own.
        // A route's own cells are what most of its cells join onto.
        const auto links = planLinks(snapshot.paths, snapshot.plan.cells);

        for (std::size_t index = 0; index < snapshot.plan.cells.size();
             ++index)
        {
            const auto cell = snapshot.plan.cells[index];

            if (!onCanvas(cell, canvas, snapshot))
            {
                continue;
            }

            renderer.drawTexture(
                atlases.oneByOne,
                roadTile(links[index]),
                tileSpriteBounds(cell, snapshot.camera),
                tint);
        }
    }

    void GridScene::drawGhost(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases) const
    {
        const auto &ghost = snapshot.ghost;

        if (!ghost.visible)
        {
            return;
        }

        const auto kind = buildingKindOf(ghost.tool);
        const auto footprint =
            kind.has_value() ? footprintOf(*kind) : Footprint{};
        const auto sheet = toolAtlasOf(ghost.tool);

        const auto bounds = spriteBounds(
            sheet,
            blockAnchor(ghost.at, footprint, snapshot.camera),
            snapshot.camera);

        if (!overlaps(bounds, canvas))
        {
            return;
        }

        // A road ghost shows the junction it would become.
        // Worked out the same way a laid one is.
        // So what is previewed is what is placed.
        renderer.drawTexture(
            atlases.of(sheet),
            toolTile(ghost.tool, linksAt(snapshot.paths, ghost.at)),
            bounds,
            ghost.valid ? kGhostly : kBlocked);

        // A border round exactly the cells the click will take.
        // A faint sprite says roughly where; an edge says what.
        // Traced round the block's own diamond box.
        // Not the sprite's box: the claim is cells, never headroom.
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

} // namespace antwika::game
