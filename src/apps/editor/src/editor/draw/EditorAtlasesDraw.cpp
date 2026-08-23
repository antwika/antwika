#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/TilemapView.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/voxel/VoxelPosition.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/gfx/SizeF.hpp>

#include "antwika/editor/Editor.hpp"

namespace
{

    [[nodiscard]] std::optional<antwika::voxel::Kind> getHoveredKind(
        const antwika::ui::Frame &frame)
    {
        for (const auto kind : antwika::voxel::kEveryKind)
        {
            if (frame.interactions.hoveredWidget
                == antwika::editor::getKindWidget(kind))
            {
                return kind;
            }
        }

        return std::nullopt;
    }

}

namespace antwika::editor
{

    void Editor::drawAtlasesView(
        const ui::Frame &frame,
        const std::chrono::time_point<std::chrono::system_clock> startedAt)
    {
        const auto where = gridRect();
        const auto fadeOthers = selectedTile.has_value()
                             && selectedEdges.has_value();
        const auto hoveredTileKind = getHoveredKind(frame);
        const auto sheetClip = sheetClipRect();
        const auto frameArea = frameRect();

        viewportRenderer.drawRect(sheetClip, kPanelColor);
        viewportRenderer.drawRect(frameArea, kPanelColor);
        {
            const auto scope =
                viewportRenderer.clipScope(sheetClip);


            for (std::uint32_t row = 0; row < document.map.tilemap.rows;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column < document.map.tilemap.columns;
                     ++column)
                {
                    const auto tile = document.map.tilemap.getEntryAt(column, row);

                    if (!tile.has_value())
                    {
                        const auto place = getTilePlace(
                            document.map.tilemap, column, row, where);
                        const auto middle =
                            antwika::gfx::PointF{
                                place.originPoint.x
                                    + (place.size.width
                                       / 2.0F),
                                place.originPoint.y
                                    + (place.size.height
                                       / 2.0F)};
                        const auto arm = std::min(
                            place.size.width,
                            place.size.height)
                            * kEmptyMarkerArmFraction;

                        viewportRenderer.drawLine(
                            {middle.x - arm, middle.y},
                            {middle.x + arm, middle.y},
                            kEmptyCellMarkerColor);
                        viewportRenderer.drawLine(
                            {middle.x, middle.y - arm},
                            {middle.x, middle.y + arm},
                            kEmptyCellMarkerColor);

                        continue;
                    }

                    const auto tileShown =
                        (!fadeOthers || selectionAllows(*tile))
                        && (!hoveredTileKind.has_value()
                            || activeRules().kindOf(*tile)
                                   == *hoveredTileKind);
                    const auto size = tilemap::tileSizeOf(
                        tile->atlas);
                    const auto place = getTilePlace(
                        document.map.tilemap, column, row, where);

                    viewportRenderer.drawTexture(
                        *atlasSheets.getChecker(tile->atlas),
                        antwika::gfx::RectF(
                            {0.0F, 0.0F},
                            {static_cast<float>(size.width),
                             static_cast<float>(
                                 size.height)}),
                        place,
                        kWhiteColor);
                    const auto *strolling =
                        decor::decorOf(
                            document.map.decor, *tile);

                    viewportRenderer.drawTexture(
                        *atlasSheets.getTexture(tile->atlas),
                        tilemap::getTileSource(
                            strolling != nullptr
                                       ? decor::decorFrameAt(
                                      *strolling, tick)
                                : *tile),
                        place,
                        tileShown ? kWhiteColor : kDisabledTintColor);
                }
            }

            for (std::uint32_t column = 0;
                 column <= document.map.tilemap.columns;
                 ++column)
            {
                const auto originX =
                    where.originPoint.x
                    + (where.size.width
                       * static_cast<float>(column)
                       / static_cast<float>(
                           document.map.tilemap.columns));

                viewportRenderer.drawLine(
                    {originX, where.originPoint.y},
                    {originX, where.originPoint.y + where.size.height},
                    kGridLineColor);
            }

            for (std::uint32_t row = 0;
                 row <= document.map.tilemap.rows;
                 ++row)
            {
                const auto originY =
                    where.originPoint.y
                    + (where.size.height
                       * static_cast<float>(row)
                       / static_cast<float>(document.map.tilemap.rows));

                viewportRenderer.drawLine(
                    {where.originPoint.x, originY},
                    {where.originPoint.x + where.size.width, originY},
                    kGridLineColor);
            }

            if (selectedTile.has_value())
            {
                const auto fromPoint =
                    getTileCenter(document.map.tilemap, where, *selectedTile);

                for (const auto edge : tilemap::kEveryTileEdge)
                {
                    for (const auto neighbor :
                         activeRules().getAllowed(*selectedTile, edge))
                    {
                        const auto toPoint = getTileCenter(
                            document.map.tilemap, where, neighbor);

                        if (fromPoint.has_value()
                            && toPoint.has_value())
                        {
                            viewportRenderer.drawLine(
                                *fromPoint,
                                *toPoint,
                                edge.edge
                                        == antwika::voxel::
                                            EdgeKind::Interior
                                    ? kInteriorRuleLineColor
                                    : kBoundaryRuleLineColor);
                        }
                    }
                }

                const auto outlineTile =
                    [this, where](
                        const tilemap::Tile tile, const gfx::Color tone)
                {
                    const auto stands =
                        tilemap::getCellHoldingTile(document.map.tilemap, tile);

                    if (!stands.has_value())
                    {
                        return;
                    }

                    for (const auto bar : getOutlineRects(
                             getTilePlace(
                                 document.map.tilemap,
                                 stands->column,
                                 stands->row,
                                 where),
                             kBorderThick))
                    {
                        viewportRenderer.drawRect(bar, tone);
                    }
                };

                outlineTile(*selectedTile, kSelectionAccentColor);

                if (!assignMode.basePicking)
                {
                    for (std::uint32_t row = 0;
                         row < document.map.tilemap.rows;
                         ++row)
                    {
                        for (std::uint32_t column = 0;
                             column < document.map.tilemap.columns;
                             ++column)
                        {
                            const auto neighbourTile =
                                document.map.tilemap.getEntryAt(column, row);

                            if (!neighbourTile.has_value()
                                || *neighbourTile == *selectedTile
                                || !mayAdjoin(*selectedTile, *neighbourTile))
                            {
                                continue;
                            }

                            outlineTile(*neighbourTile, kInteriorRuleLineColor);
                        }
                    }
                }

                if (isDecorLayer())
                {
                    const auto *decor =
                        decor::decorOf(
                            document.map.decor, *selectedTile);

                    for (const auto base :
                         decor != nullptr
                                ? decor->allowedBaseTiles
                                : std::vector<tilemap::Tile>{})
                    {
                        outlineTile(base, kRuleLineColor);
                    }
                }

                if (transitionPicked.has_value()
                    && *transitionPicked
                           < document.map.transitions.size())
                {
                    const auto &transition = document.map.transitions.at(
                        *transitionPicked);

                    for (const auto &[tile, tone] :
                         {std::pair{transition.fromTile, kVariantLinkLineColor},
                          std::pair{transition.toTile, kVariantLinkLineColor},
                          std::pair{
                              transition.maskTile,
                              kBoundaryRuleLineColor},
                          std::pair{
                              transition.outputTile,
                              kSelectionAccentColor}})
                    {
                        outlineTile(tile, tone);
                    }
                }

                const auto *family =
                    getGroupLedBy(document.map.familyGroups, *selectedTile);

                if (family == nullptr)
                {
                    family = getGroupContaining(
                        document.map.familyGroups, *selectedTile);
                }

                if (family != nullptr && !isDecorLayer())
                {
                    auto tiles =
                        std::vector<tilemap::Tile>{family->canonicalTile};

                    for (const auto &member : family->variants)
                    {
                        tiles.push_back(member.tile);
                    }

                    for (const auto tile : tiles)
                    {
                        if (tile != *selectedTile)
                        {
                            outlineTile(tile, kVariantLinkLineColor);
                        }
                    }
                }

            }

        }

        if (selectedTile.has_value())
        {
            {
                const auto scope =
                    viewportRenderer.clipScope(frameArea);


                {
                    const auto size =
                        tilemap::tileSizeOf(selectedTile->atlas);

                    viewportRenderer.drawTexture(
                        *atlasSheets.getChecker(selectedTile->atlas),
                        antwika::gfx::RectF(
                            {0.0F, 0.0F},
                            {static_cast<float>(size.width),
                             static_cast<float>(
                                 size.height)}),
                        getInspectedTileRect(frameRect(), *selectedTile),
                        kWhiteColor);
                }

                viewportRenderer.drawTexture(
                    *atlasSheets.getTexture(selectedTile->atlas),
                    tilemap::getTileSource(editedTile()),
                    getInspectedTileRect(frameRect(), *selectedTile),
                    kWhiteColor);

                for (const auto bar : getOutlineRects(
                         getInspectedTileRect(frameRect(), *selectedTile),
                         kBorderThick))
                {
                    viewportRenderer.drawRect(bar, kTextColor);
                }

                for (const auto corner : voxel::kEveryCorner)
                {
                    const auto cornerRule =
                        activeRules().getCorner(*selectedTile, corner);

                    viewportRenderer.drawRect(
                        getCornerPlace(frameRect(), corner),
                        !cornerRule.has_value() ? kTextColor
                        : *cornerRule           ? kCornerFilledMarkerColor
                                           : kCornerEmptyMarkerColor);
                }

                for (const auto edge : tilemap::kEveryTileEdge)
                {
                    const auto ink =
                        selectedEdges.has_value()
                                && covers(*selectedEdges, edge)
                            ? kSelectionAccentColor
                            : (activeRules().isForbidden(
                                   *selectedTile, edge)
                                   ? kForbiddenMarkerColor
                                   : kTextColor);
                    const auto where =
                        getMarkerPlace(frameRect(), edge);

                    viewportRenderer.drawRect(where, ink);

                    if (!activeRules().allowsBoundary(
                            *selectedTile, edge))
                    {
                        continue;
                    }

                    const auto isHorizontal =
                        where.size.width > where.size.height;

                    viewportRenderer.drawRect(
                        antwika::gfx::RectF(
                            where.originPoint,
                            antwika::gfx::SizeF{
                                isHorizontal ? where.size.width
                                             : kBoundaryBandThickness,
                                isHorizontal ? kBoundaryBandThickness
                                             : where.size.height}),
                        kBoundaryMarkerColor);
                }

                for (const auto side : voxel::kEverySide)
                {
                    const auto pair = bothEdgesOf(side);
                    const auto forbidden = std::ranges::all_of(
                        edgesIn(pair),
                        [this](const tilemap::TileEdge edge) {
                            return activeRules().isForbidden(
                                *selectedTile, edge);
                        });

                    viewportRenderer.drawRect(
                        getBothMarkerPlace(frameRect(), side),
                        selectedEdges == pair  ? kSelectionAccentColor
                        : forbidden            ? kForbiddenMarkerColor
                                               : kTextColor);
                }

            }
        }

        if (selectedTile.has_value())
        {
            const auto face =
                getInspectedTileRect(frameRect(), *selectedTile);
            const auto pixel =
                tile::pixelAt(*selectedTile, face, pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                const auto markedTiles =
                    !lineFromCell.has_value()
                        ? std::vector{*pixel}
                    : settings.paint == map::Paint::Rect
                        ? tile::getRectPixels(
                              *lineFromCell, *pixel)
                    : settings.paint == map::Paint::Circle
                        ? tile::getCirclePixels(
                              *lineFromCell, *pixel)
                    : settings.paint == map::Paint::Line
                        ? tile::getLinePixels(*lineFromCell, *pixel)
                        : std::vector{*pixel};

                for (const auto one : markedTiles)
                {
                    for (const auto bar : getOutlineRects(
                             tile::getPixelPlace(
                                 *selectedTile, face, one),
                             kCursorThickness))
                    {
                        viewportRenderer.drawRect(bar, kCursorColor);
                    }
                }
            }
        }

        if (dragFromCell.has_value()
            && document.map.tilemap
                   .getEntryAt(dragFromCell->column, dragFromCell->row)
                   .has_value())
        {
            const auto tile = document.map.tilemap.getEntryAt(
                dragFromCell->column, dragFromCell->row);
            const auto cell = getTilePlace(
                document.map.tilemap,
                dragFromCell->column,
                dragFromCell->row,
                where);

            viewportRenderer.drawTexture(
                *atlasSheets.getTexture(tile->atlas),
                tilemap::getTileSource(*tile),
                antwika::gfx::RectF(
                    {pointer.pointerOnCanvas.x - (cell.size.width / 2.0F),
                     pointer.pointerOnCanvas.y
                         - (cell.size.height / 2.0F)},
                    cell.size),
                kWhiteColor);
        }

        finishView(frame, startedAt);

        return;
    }

}
