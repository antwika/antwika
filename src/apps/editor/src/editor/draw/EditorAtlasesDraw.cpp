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

    [[nodiscard]] std::optional<antwika::voxel::Kind> hoveredKind(
        const antwika::ui::Frame &frame)
    {
        for (const auto kind : antwika::voxel::kEveryKind)
        {
            if (frame.interactions.hoveredWidget
                == antwika::editor::kindWidget(kind))
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
        const auto hoveredTileKind = hoveredKind(frame);
        const auto sheetClip = sheetClipRect();
        const auto frameArea = frameRect();

        viewportRenderer.drawRect(sheetClip, kPanelColor);
        viewportRenderer.drawRect(frameArea, kPanelColor);
        {
            const auto scope =
                viewportRenderer.clipScope(sheetClip);


            for (std::uint32_t row = 0; row < map.tilemap.rows;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.tilemap.columns;
                     ++column)
                {
                    const auto tile = map.tilemap.at(column, row);

                    if (!tile.has_value())
                    {
                        const auto place = tilePlace(
                            map.tilemap, column, row, where);
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
                    const auto place = tilePlace(
                        map.tilemap, column, row, where);

                    viewportRenderer.drawTexture(
                        *atlasSheets.checker(tile->atlas),
                        antwika::gfx::RectF(
                            {0.0F, 0.0F},
                            {static_cast<float>(size.width),
                             static_cast<float>(
                                 size.height)}),
                        place,
                        kWhiteColor);
                    const auto *strolling =
                        decor::decorOf(
                            map.decor, *tile);

                    viewportRenderer.drawTexture(
                        *atlasSheets.texture(tile->atlas),
                        tilemap::tileSource(
                            strolling != nullptr
                                       ? decor::decorFrameAt(
                                      *strolling, tick)
                                : *tile),
                        place,
                        tileShown ? kWhiteColor : kDisabledTintColor);
                }
            }

            for (std::uint32_t column = 0;
                 column <= map.tilemap.columns;
                 ++column)
            {
                const auto originX =
                    where.originPoint.x
                    + (where.size.width
                       * static_cast<float>(column)
                       / static_cast<float>(
                           map.tilemap.columns));

                viewportRenderer.drawLine(
                    {originX, where.originPoint.y},
                    {originX, where.originPoint.y + where.size.height},
                    kGridLineColor);
            }

            for (std::uint32_t row = 0;
                 row <= map.tilemap.rows;
                 ++row)
            {
                const auto originY =
                    where.originPoint.y
                    + (where.size.height
                       * static_cast<float>(row)
                       / static_cast<float>(map.tilemap.rows));

                viewportRenderer.drawLine(
                    {where.originPoint.x, originY},
                    {where.originPoint.x + where.size.width, originY},
                    kGridLineColor);
            }

            if (selectedTile.has_value())
            {
                const auto fromPoint =
                    tileCenter(map.tilemap, where, *selectedTile);

                for (const auto edge : tilemap::kEveryTileEdge)
                {
                    for (const auto neighbor :
                         activeRules().allowed(*selectedTile, edge))
                    {
                        const auto toPoint = tileCenter(
                            map.tilemap, where, neighbor);

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
                        tilemap::cellHoldingTile(map.tilemap, tile);

                    if (!stands.has_value())
                    {
                        return;
                    }

                    for (const auto bar : outlineRects(
                             tilePlace(
                                 map.tilemap,
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
                         row < map.tilemap.rows;
                         ++row)
                    {
                        for (std::uint32_t column = 0;
                             column < map.tilemap.columns;
                             ++column)
                        {
                            const auto neighbourTile =
                                map.tilemap.at(column, row);

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
                            map.decor, *selectedTile);

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
                           < map.transitions.size())
                {
                    const auto &transition = map.transitions.at(
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
                    groupLedBy(map.familyGroups, *selectedTile);

                if (family == nullptr)
                {
                    family = groupContaining(
                        map.familyGroups, *selectedTile);
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
                        *atlasSheets.checker(selectedTile->atlas),
                        antwika::gfx::RectF(
                            {0.0F, 0.0F},
                            {static_cast<float>(size.width),
                             static_cast<float>(
                                 size.height)}),
                        inspectedTileRect(frameRect(), *selectedTile),
                        kWhiteColor);
                }

                viewportRenderer.drawTexture(
                    *atlasSheets.texture(selectedTile->atlas),
                    tilemap::tileSource(editedTile()),
                    inspectedTileRect(frameRect(), *selectedTile),
                    kWhiteColor);

                for (const auto bar : outlineRects(
                         inspectedTileRect(frameRect(), *selectedTile),
                         kBorderThick))
                {
                    viewportRenderer.drawRect(bar, kTextColor);
                }

                for (const auto corner : voxel::kEveryCorner)
                {
                    const auto cornerRule =
                        activeRules().corner(*selectedTile, corner);

                    viewportRenderer.drawRect(
                        cornerPlace(frameRect(), corner),
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
                        markerPlace(frameRect(), edge);

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
                        bothMarkerPlace(frameRect(), side),
                        selectedEdges == pair  ? kSelectionAccentColor
                        : forbidden            ? kForbiddenMarkerColor
                                               : kTextColor);
                }

            }
        }

        if (selectedTile.has_value())
        {
            const auto face =
                inspectedTileRect(frameRect(), *selectedTile);
            const auto pixel =
                tile::pixelAt(*selectedTile, face, pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                const auto markedTiles =
                    !lineFromCell.has_value()
                        ? std::vector{*pixel}
                    : paintMode == map::Paint::Rect
                        ? tile::rectPixels(
                              *lineFromCell, *pixel)
                    : paintMode == map::Paint::Circle
                        ? tile::circlePixels(
                              *lineFromCell, *pixel)
                    : paintMode == map::Paint::Line
                        ? tile::linePixels(*lineFromCell, *pixel)
                        : std::vector{*pixel};

                for (const auto one : markedTiles)
                {
                    for (const auto bar : outlineRects(
                             tile::pixelPlace(
                                 *selectedTile, face, one),
                             kCursorThickness))
                    {
                        viewportRenderer.drawRect(bar, kCursorColor);
                    }
                }
            }
        }

        if (dragFromCell.has_value()
            && map.tilemap
                   .at(dragFromCell->column, dragFromCell->row)
                   .has_value())
        {
            const auto tile = map.tilemap.at(
                dragFromCell->column, dragFromCell->row);
            const auto cell = tilePlace(
                map.tilemap,
                dragFromCell->column,
                dragFromCell->row,
                where);

            viewportRenderer.drawTexture(
                *atlasSheets.texture(tile->atlas),
                tilemap::tileSource(*tile),
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
