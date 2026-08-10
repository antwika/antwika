#include "antwika/game/GridScene.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Rect.hpp>

#include "antwika/game/Direction.hpp"
#include "antwika/game/Footprint.hpp"
#include "antwika/game/FootprintOutline.hpp"
#include "antwika/game/IsoProjection.hpp"
#include "antwika/game/OverlayField.hpp"
#include "antwika/game/OverlayLabel.hpp"
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

        constexpr Color kUntinted{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr Color kGhostly{
            .red = 255, .green = 255, .blue = 255, .alpha = 110};

        constexpr Color kBlocked{
            .red = 255, .green = 90, .blue = 90, .alpha = 110};

        constexpr Color kGhostEdge{
            .red = 255, .green = 255, .blue = 255, .alpha = 220};

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
        } // GCOVR_EXCL_LINE

        struct BlockArt final
        {
            Cell at;

            BuildingKind kind = BuildingKind::House;

            Rect tile;
        };

        [[nodiscard]] std::vector<BlockArt> blocksOf(
            const SceneSnapshot &snapshot, const AtlasSpecs &specs)
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
                        .tile = buildingTile(specs, sprite.kind)});
                    ++building;
                }
                else
                {
                    const auto &sprite = snapshot.ruins[ruin];
                    blocks.push_back(BlockArt{
                        .at = sprite.at,
                        .kind = sprite.kind,
                        .tile = ruinTile(specs, sprite.state, sprite.kind)});
                    ++ruin;
                }
            }

            return blocks;
        } // GCOVR_EXCL_LINE

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
        } // GCOVR_EXCL_LINE

        struct PulledFlanks final
        {
            std::vector<Cell> cells;

            std::vector<std::pair<std::int32_t, Cell>> byDiagonal;
        };

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

            std::sort(pulls.cells.begin(), pulls.cells.end());

            return pulls;
        } // GCOVR_EXCL_LINE
    }

    bool GridScene::onCanvas(
        const AtlasSpecs &specs,
        Cell cell,
        Size canvas,
        const SceneSnapshot &snapshot)
    {
        return overlaps(
            tileSpriteBounds(specs, cell, snapshot.camera), canvas);
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

        const auto phase = snapshot.paused ? Progress() : subTick;

        const bool overlaid = snapshot.view != MapView::Normal;

        drawTerrain(renderer, canvas, snapshot, atlases, phase, !overlaid);

        drawOverlay(renderer, canvas, snapshot, atlases);

        if (overlaid)
        {
            for (const auto &walker : snapshot.walkers)
            {
                drawWalker(
                    renderer, canvas, walker, snapshot, atlases, phase);
            }
        }

        drawOverlayValues(renderer, canvas, snapshot, atlases);

        drawPlan(renderer, canvas, snapshot, atlases);

        drawGhost(renderer, canvas, snapshot, atlases);

        drawReadout(renderer, canvas, snapshot);
    }

    void GridScene::drawWalker(
        IRenderer &renderer,
        Size canvas,
        const WalkerSprite &walker,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases,
        Progress phase) const
    {
        const auto bounds = walkerBounds(
            atlases.specs, walker, snapshot.camera, phase);

        if (!overlaps(bounds, canvas))
        {
            return;
        }

        renderer.drawTexture(
            atlases.walker,
            walkerTile(
                atlases.specs,
                walker.facing, walkerFrame(walker, phase)),
            bounds,
            kUntinted);
    }

    void GridScene::drawTerrain(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases,
        Progress phase,
        bool withWalkers) const
    {
        const auto blocks = blocksOf(snapshot, atlases.specs);
        const auto covered = coveredCells(blocks);
        std::size_t next = 0;

        const auto depthOf = [&](const WalkerSprite &walker)
        {
            const auto from = walker.from.value_or(walker.at);

            return std::max(
                walker.at.x + walker.at.y, from.x + from.y);
        };

        std::vector<std::size_t> marching;
        std::size_t stepping = 0;

        if (withWalkers)
        {
            marching.resize(snapshot.walkers.size());

            for (std::size_t index = 0; index < marching.size(); ++index)
            {
                marching[index] = index;
            }

            std::stable_sort(
                marching.begin(),
                marching.end(),
                [&](std::size_t left, std::size_t right)
                {
                    return depthOf(snapshot.walkers[left])
                        < depthOf(snapshot.walkers[right]);
                });
        }

        const auto drawBlock = [&](const BlockArt &block)
        {
            const auto bounds = buildingSpriteBounds(atlases.specs, 
                block.at, block.kind, snapshot.camera);

            if (!overlaps(bounds, canvas))
            {
                return;
            }

            renderer.drawTexture(
                atlases.of(buildingAtlasOf(block.kind)),
                block.tile,
                bounds,
                kUntinted);
        };

        const auto drawGround = [&](const Cell cell)
        {
            if (!onCanvas(atlases.specs, cell, canvas, snapshot))
            {
                return;
            }

            const auto bounds = tileSpriteBounds(
                atlases.specs,
                cell, snapshot.camera);

            const auto ground = groundTile(atlases.specs);

            renderer.drawTexture(atlases.oneByOne, ground, bounds, kUntinted);

            if (paved(snapshot.paths, cell))
            {
                renderer.drawTexture(
                    atlases.oneByOne,
                    roadTile(atlases.specs, linksAt(snapshot.paths, cell)),
                    bounds,
                    kUntinted);
            }
        };

        const auto pulls = pulledFlanks(blocks, covered);
        std::size_t drawn = 0;

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

                if (std::binary_search(
                        covered.begin(), covered.end(), cell))
                {
                    continue;
                }

                if (std::binary_search(
                        pulls.cells.begin(), pulls.cells.end(), cell))
                {
                    continue;
                }

                drawGround(cell);
            }

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

            while (stepping < marching.size()
                   && depthOf(snapshot.walkers[marching[stepping]])
                       <= diagonal)
            {
                drawWalker(
                    renderer,
                    canvas,
                    snapshot.walkers[marching[stepping]],
                    snapshot,
                    atlases,
                    phase);
                ++stepping;
            }
        }

        while (next < blocks.size())
        {
            drawBlock(blocks[next]);
            ++next;
        }

        while (stepping < marching.size())
        {
            drawWalker(
                renderer,
                canvas,
                snapshot.walkers[marching[stepping]],
                snapshot,
                atlases,
                phase);
            ++stepping;
        }
    }

    void GridScene::drawOverlay(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases) const
    {
        if (snapshot.view == MapView::Normal)
        {
            return;
        }

        const auto ink = overlayColour(snapshot.view);

        for (std::int32_t y = 0; y < snapshot.extent.height; ++y)
        {
            for (std::int32_t x = 0; x < snapshot.extent.width; ++x)
            {
                const Cell cell{.x = x, .y = y};

                if (!onCanvas(atlases.specs, cell, canvas, snapshot))
                {
                    continue;
                }

                const auto bounds =
                    tileSpriteBounds(atlases.specs, cell, snapshot.camera);

                renderer.drawTexture(
                    atlases.oneByOne,
                    groundTile(atlases.specs),
                    bounds,
                    kOverlayScrim);

                const auto found = snapshot.overlay.find(cell);

                if (found == snapshot.overlay.end())
                {
                    continue;
                }

                const auto strength = kOverlayFaintest
                    + (255 - kOverlayFaintest) * found->second / 100;

                renderer.drawTexture(
                    atlases.oneByOne,
                    groundTile(atlases.specs),
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

    void GridScene::drawOverlayValues(
        IRenderer &renderer,
        Size canvas,
        const SceneSnapshot &snapshot,
        const AtlasTextures &atlases) const
    {
        if (snapshot.view == MapView::Normal)
        {
            return;
        }

        for (const auto &[cell, value] : snapshot.overlay)
        {
            if (!onCanvas(atlases.specs, cell, canvas, snapshot))
            {
                continue;
            }

            const auto text = std::to_string(value);
            const auto label =
                overlayLabelFor(text, cell, snapshot.camera);

            if (!label.has_value())
            {
                continue;
            }

            renderer.drawText(
                label->origin, text, label->scale, kOverlayInk);
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
        const auto tint = snapshot.plan.valid ? kGhostly : kBlocked;

        const auto links = planLinks(snapshot.paths, snapshot.plan.cells);
        const auto kind = buildingKindOf(snapshot.ghost.tool);

        for (std::size_t index = 0; index < snapshot.plan.cells.size();
             ++index)
        {
            const auto cell = snapshot.plan.cells[index];

            if (!onCanvas(atlases.specs, cell, canvas, snapshot))
            {
                continue;
            }

            if (kind.has_value())
            {
                renderer.drawTexture(
                    atlases.of(buildingAtlasOf(*kind)),
                    buildingTile(atlases.specs, *kind),
                    buildingSpriteBounds(
                        atlases.specs,
                        cell, *kind, snapshot.camera),
                    tint);

                continue;
            }

            renderer.drawTexture(
                atlases.oneByOne,
                roadTile(atlases.specs, links[index]),
                tileSpriteBounds(atlases.specs, cell, snapshot.camera),
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

        const auto bounds = spriteBounds(atlases.specs, 
            sheet,
            blockAnchor(ghost.at, footprint, snapshot.camera),
            snapshot.camera);

        if (!overlaps(bounds, canvas))
        {
            return;
        }

        renderer.drawTexture(
            atlases.of(sheet),
            toolTile(
                atlases.specs,
                ghost.tool, linksAt(snapshot.paths, ghost.at)),
            bounds,
            ghost.valid ? kGhostly : kBlocked);

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

}
