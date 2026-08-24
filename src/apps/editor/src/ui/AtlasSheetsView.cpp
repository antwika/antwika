#include "antwika/editor/ui/AtlasSheetsView.hpp"

#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/editor/ui/TilemapView.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/SizeF.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxel/VoxelPosition.hpp>

#include "antwika/editor/LayerEdit.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

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

    void AtlasSheetsView::draw(
        const ViewContext &viewContext, const ui::Frame &frame)
    {
        auto &stroke = viewContext.workbench.stroke;
        auto &sheetView = viewContext.workbench.sheetView;
        auto &pointer = viewContext.workbench.pointer;
        auto &preferences = viewContext.workbench.preferences;
        auto &assignMode = viewContext.workbench.assignMode;
        auto &transition = viewContext.workbench.transition;
        const auto chosenLayer = viewContext.workbench.chosenLayer;
        auto &atlasSheets = viewContext.render.atlasSheets;
        auto &worldMeshes = viewContext.render.worldMeshes;
        auto &viewportRenderer = viewContext.render.viewportRenderer;
        auto &drawnMap = viewContext.document.map;
        const auto tick = viewContext.tick;

        const auto where = sheetView.getGridRect(
            drawnMap.tilemap);
        const auto fadeOthers = stroke.selectedTile.has_value()
                             && stroke.selectedEdges.has_value();
        const auto hoveredTileKind = getHoveredKind(frame);
        const auto sheetClip = sheetView.getClipRect();
        const auto frameArea = sheetView.getFrameRect();

        viewportRenderer.drawRect(sheetClip, kPanelColor);
        viewportRenderer.drawRect(frameArea, kPanelColor);
        {
            const auto scope =
                viewportRenderer.clipScope(sheetClip);


            for (std::uint32_t row = 0; row < drawnMap.tilemap.rows;
                 ++row)
            {
                for (std::uint32_t column = 0;
                     column < drawnMap.tilemap.columns;
                     ++column)
                {
                    const auto tile = drawnMap.tilemap.getEntryAt(column, row);

                    if (!tile.has_value())
                    {
                        const auto place = getTilePlace(
                            drawnMap.tilemap, column, row, where);
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
                        (!fadeOthers || stroke.allows(getActiveRules(
                            drawnMap, chosenLayer), *tile))
                        && (!hoveredTileKind.has_value()
                            || getActiveRules(
                drawnMap, chosenLayer).kindOf(*tile)
                                   == *hoveredTileKind);
                    const auto size = tilemap::tileSizeOf(
                        tile->atlas);
                    const auto place = getTilePlace(
                        drawnMap.tilemap, column, row, where);

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
                            drawnMap.decor, *tile);

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
                 column <= drawnMap.tilemap.columns;
                 ++column)
            {
                const auto originX =
                    where.originPoint.x
                    + (where.size.width
                       * static_cast<float>(column)
                       / static_cast<float>(
                           drawnMap.tilemap.columns));

                viewportRenderer.drawLine(
                    {originX, where.originPoint.y},
                    {originX, where.originPoint.y + where.size.height},
                    kGridLineColor);
            }

            for (std::uint32_t row = 0;
                 row <= drawnMap.tilemap.rows;
                 ++row)
            {
                const auto originY =
                    where.originPoint.y
                    + (where.size.height
                       * static_cast<float>(row)
                       / static_cast<float>(drawnMap.tilemap.rows));

                viewportRenderer.drawLine(
                    {where.originPoint.x, originY},
                    {where.originPoint.x + where.size.width, originY},
                    kGridLineColor);
            }

            if (stroke.selectedTile.has_value())
            {
                const auto fromPoint =
                    getTileCenter(drawnMap.tilemap, where, *
                        stroke.selectedTile);

                for (const auto edge : tilemap::kEveryTileEdge)
                {
                    for (const auto neighbor :
                         getActiveRules(
                             drawnMap, 
                                 chosenLayer).getAllowedTiles(*stroke.selectedTile, edge))
                    {
                        const auto toPoint = getTileCenter(
                            drawnMap.tilemap, where, neighbor);

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
                    [&, where](
                        const tilemap::Tile tile, const gfx::Color tone)
                {
                    const auto stands =
                        tilemap::getCellHoldingTile(drawnMap.tilemap, tile);

                    if (!stands.has_value())
                    {
                        return;
                    }

                    for (const auto bar : getOutlineRects(
                             getTilePlace(
                                 drawnMap.tilemap,
                                 stands->column,
                                 stands->row,
                                 where),
                             kBorderThick))
                    {
                        viewportRenderer.drawRect(bar, tone);
                    }
                };

                outlineTile(*stroke.selectedTile, kSelectionAccentColor);

                if (!assignMode.basePicking)
                {
                    for (std::uint32_t row = 0;
                         row < drawnMap.tilemap.rows;
                         ++row)
                    {
                        for (std::uint32_t column = 0;
                             column < drawnMap.tilemap.columns;
                             ++column)
                        {
                            const auto neighbourTile =
                                drawnMap.tilemap.getEntryAt(column, row);

                            if (!neighbourTile.has_value()
                                || *neighbourTile == *stroke.selectedTile
                                || !isAdjoining(
                                    isDecorLayer(chosenLayer)
                                        ? drawnMap.decorRules
                                        : worldMeshes.getRules(),
                                    *stroke.selectedTile,
                                    *neighbourTile))
                            {
                                continue;
                            }

                            outlineTile(*neighbourTile, kInteriorRuleLineColor);
                        }
                    }
                }

                if (isDecorLayer(chosenLayer))
                {
                    const auto *decor =
                        decor::decorOf(
                            drawnMap.decor, *
                                stroke.selectedTile);

                    for (const auto base :
                         decor != nullptr
                                ? decor->allowedBaseTiles
                                : std::vector<tilemap::Tile>{})
                    {
                        outlineTile(base, kRuleLineColor);
                    }
                }

                if (transition.chosenIndex.has_value()
                    && *transition.chosenIndex
                           < drawnMap.transitions.size())
                {
                    const auto &pickedTransition =
                        drawnMap.transitions.at(*
                            transition.chosenIndex);

                    for (const auto &[tile, tone] :
                         {std::pair{
                              pickedTransition.fromTile,
                              kVariantLinkLineColor},
                          std::pair{
                              pickedTransition.toTile,
                              kVariantLinkLineColor},
                          std::pair{
                              pickedTransition.maskTile,
                              kBoundaryRuleLineColor},
                          std::pair{
                              pickedTransition.outputTile,
                              kSelectionAccentColor}})
                    {
                        outlineTile(tile, tone);
                    }
                }

                const auto *family =
                    getGroupLedBy(drawnMap.familyGroups, *
                        stroke.selectedTile);

                if (family == nullptr)
                {
                    family = getGroupContaining(
                        drawnMap.familyGroups, *stroke.selectedTile);
                }

                if (family != nullptr && !isDecorLayer(chosenLayer))
                {
                    auto tiles =
                        std::vector<tilemap::Tile>{family->canonicalTile};

                    for (const auto &member : family->variants)
                    {
                        tiles.push_back(member.tile);
                    }

                    for (const auto tile : tiles)
                    {
                        if (tile != *stroke.selectedTile)
                        {
                            outlineTile(tile, kVariantLinkLineColor);
                        }
                    }
                }

            }

        }

        if (stroke.selectedTile.has_value())
        {
            {
                const auto scope =
                    viewportRenderer.clipScope(frameArea);


                {
                    const auto size =
                        tilemap::tileSizeOf(stroke.selectedTile->atlas);

                    viewportRenderer.drawTexture(
                        *atlasSheets.getChecker(stroke.selectedTile->atlas),
                        antwika::gfx::RectF(
                            {0.0F, 0.0F},
                            {static_cast<float>(size.width),
                             static_cast<float>(
                                 size.height)}),
                        getInspectedTileRect(sheetView.getFrameRect(), *stroke.selectedTile),
                        kWhiteColor);
                }

                viewportRenderer.drawTexture(
                    *atlasSheets.getTexture(stroke.selectedTile->atlas),
                    tilemap::getTileSource(getEditedTile(
                        drawnMap, chosenLayer,
                        stroke,
                        assignMode)),
                    getInspectedTileRect(sheetView.getFrameRect(), *stroke.selectedTile),
                    kWhiteColor);

                for (const auto bar : getOutlineRects(
                         getInspectedTileRect(sheetView.getFrameRect(), *stroke.selectedTile),
                         kBorderThick))
                {
                    viewportRenderer.drawRect(bar, kTextColor);
                }

                for (const auto corner : voxel::kEveryCorner)
                {
                    const auto cornerRule =
                        getActiveRules(
                            drawnMap, chosenLayer).getCorner(*stroke.selectedTile, corner);

                    viewportRenderer.drawRect(
                        getCornerPlace(sheetView.getFrameRect(), corner),
                        !cornerRule.has_value() ? kTextColor
                        : *cornerRule           ? kCornerFilledMarkerColor
                                           : kCornerEmptyMarkerColor);
                }

                for (const auto edge : tilemap::kEveryTileEdge)
                {
                    const auto ink =
                        stroke.selectedEdges.has_value()
                                && covers(*stroke.selectedEdges, edge)
                            ? kSelectionAccentColor
                            : (getActiveRules(
                drawnMap, chosenLayer).isForbidden(
                                   *stroke.selectedTile, edge)
                                   ? kForbiddenMarkerColor
                                   : kTextColor);
                    const auto where =
                        getMarkerPlace(sheetView.getFrameRect(), edge);

                    viewportRenderer.drawRect(where, ink);

                    if (!getActiveRules(
                drawnMap, chosenLayer).allowsBoundary(
                            *stroke.selectedTile, edge))
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
                        [&](const tilemap::TileEdge edge) {
                            return getActiveRules(
                drawnMap, chosenLayer).isForbidden(
                                *stroke.selectedTile, edge);
                        });

                    viewportRenderer.drawRect(
                        getBothMarkerPlace(sheetView.getFrameRect(), side),
                        stroke.selectedEdges == pair  ? kSelectionAccentColor
                        : forbidden            ? kForbiddenMarkerColor
                                               : kTextColor);
                }

            }
        }

        if (stroke.selectedTile.has_value())
        {
            const auto face =
                getInspectedTileRect(sheetView.getFrameRect(), *stroke.selectedTile);
            const auto pixel =
                tile::pixelAt(*stroke.selectedTile, face, pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                const auto markedTiles =
                    !stroke.lineFromCell.has_value()
                        ? std::vector{*pixel}
                    : preferences.paint == map::Paint::Rect
                        ? tile::getRectPixels(
                              *stroke.lineFromCell, *pixel)
                    : preferences.paint == map::Paint::Circle
                        ? tile::getCirclePixels(
                              *stroke.lineFromCell, *pixel)
                    : preferences.paint == map::Paint::Line
                        ? tile::getLinePixels(*stroke.lineFromCell, *pixel)
                        : std::vector{*pixel};

                for (const auto one : markedTiles)
                {
                    for (const auto bar : getOutlineRects(
                             tile::getPixelPlace(
                                 *stroke.selectedTile, face, one),
                             kCursorThickness))
                    {
                        viewportRenderer.drawRect(bar, kCursorColor);
                    }
                }
            }
        }

        if (stroke.dragFromCell.has_value()
            && drawnMap.tilemap
                   .getEntryAt(stroke.dragFromCell->column, stroke.dragFromCell->row)
                   .has_value())
        {
            const auto tile = drawnMap.tilemap.getEntryAt(
                stroke.dragFromCell->column, stroke.dragFromCell->row);
            const auto cell = getTilePlace(
                drawnMap.tilemap,
                stroke.dragFromCell->column,
                stroke.dragFromCell->row,
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

        return;
    }


    bool AtlasSheetsView::claims(
        const map::View shownView, const bool playing) const noexcept
    {
        return !playing && shownView == map::View::Atlases;
    }

    std::string AtlasSheetsView::getStatusText(
        const ViewContext &viewContext) const
    {
        return !viewContext.workbench.stroke.selectedEdges.has_value()
                           && viewContext.workbench.stroke.selectedTile.has_value()
                       ? "shift n, w, r give this tile a kind "
                         "- the buttons give it a facing"
                   : !viewContext.workbench.stroke.selectedEdges.has_value()
                       ? "click a tile to look at it, drag to "
                         "swap, del takes it out"
                   : viewContext.workbench.stroke.selectedTile.has_value()
                           && viewContext.workbench.stroke.isForbidden(getActiveRules(
                    viewContext.document.map,
                    viewContext.workbench.chosenLayer))
                       ? "this edge meets nothing - x opens it"
                   : viewContext.workbench.stroke.selectedTile.has_value()
                           && viewContext.workbench.stroke.allowsBoundary(getActiveRules(
                    viewContext.document.map,
                    viewContext.workbench.chosenLayer))
                       ? "may meet these, and the rim - r "
                         "rims, x shuts"
                       : "may meet these, not the rim - r "
                         "rims, x shuts";
    }


    bool AtlasSheetsView::takesPaintKeys() const noexcept
    {
        return true;
    }

    bool AtlasSheetsView::offersPaint(
        const map::Paint paint) const noexcept
    {
        return paint == map::Paint::Rect || paint == map::Paint::Circle
               || IEditorView::offersPaint(paint);
    }


    bool AtlasSheetsView::consumePress(
        const ViewContext &viewContext,
        const input::PointerButtonPressed &downPressed)
    {
auto &stroke = viewContext.workbench.stroke;
        auto &sheetView = viewContext.workbench.sheetView;
        auto &pointer = viewContext.workbench.pointer;
        auto &assignMode = viewContext.workbench.assignMode;
        auto &transition = viewContext.workbench.transition;
        const auto chosenLayer = viewContext.workbench.chosenLayer;
        auto &inkPicker = viewContext.workbench.inkPicker;
        auto &atlasSheets = viewContext.render.atlasSheets;
        auto &viewportRenderer = viewContext.render.viewportRenderer;
        auto &drawnMap = viewContext.document.map;

        if (downPressed.button == input::MouseButton::Right
            && !inkPicker.editingInk.has_value())
        {
        const auto canvasPoint = viewportRenderer.getViewport().toCanvas(
            gfx::Point{
                .x = downPressed.position.x,
                .y = downPressed.position.y});

        pointer.pointerOnCanvas = gfx::PointF{
            static_cast<float>(canvasPoint.x),
            static_cast<float>(canvasPoint.y)};

        if (stroke.selectedTile.has_value())
        {
            const auto editedTileValue = getEditedTile(
                drawnMap,
                chosenLayer,
                stroke,
                assignMode);
            const auto pixel = tile::pixelAt(
                editedTileValue,
                getInspectedTileRect(sheetView.getFrameRect(), editedTileValue),
                pointer.pointerOnCanvas);

            if (pixel.has_value())
            {
                viewContext.editSteps.pushUndo();
                tile::paint(
                    atlasSheets.sheet(editedTileValue.atlas),
                    editedTileValue,
                    *pixel,
                    gfx::Color{.alpha = 0});
                stroke.brushAtCell = pixel;
                stroke.active = true;
                stroke.erases = true;
                atlasSheets.touch();

                return true;
            }
        }

        stroke.selectedTile.reset();
        stroke.selectedEdges.reset();
        stroke.lineFromCell.reset();
        assignMode = AssignMode{};
        transition.fromTile.reset();
        transition.toTile.reset();

        return true;
        }

        if (downPressed.button != input::MouseButton::Left)
        {
            return false;
        }


        const auto projectToScreen = viewportRenderer.getViewport().toCanvas(
            gfx::Point{
                .x = downPressed.position.x, .y = downPressed.position.y});

        pointer.pointerOnCanvas = gfx::PointF{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};

        if (paintedOnAtlasPixel(viewContext))
        {
            return true;
        }

        stroke.dragFromPoint = pointer.pointerOnCanvas;
        stroke.dragFromCell = sheetView.getCellUnder(
            drawnMap.tilemap,
            pointer.pointerOnCanvas);

        return true;
    }

    bool AtlasSheetsView::paintedOnAtlasPixel(
        const ViewContext &viewContext)
    {
auto &stroke = viewContext.workbench.stroke;
        auto &sheetView = viewContext.workbench.sheetView;
        auto &pointer = viewContext.workbench.pointer;
        auto &preferences = viewContext.workbench.preferences;
        auto &assignMode = viewContext.workbench.assignMode;
        const auto chosenLayer = viewContext.workbench.chosenLayer;
        auto &inkPicker = viewContext.workbench.inkPicker;
        auto &atlasSheets = viewContext.render.atlasSheets;
        auto &drawnMap = viewContext.document.map;

        if (!stroke.selectedTile.has_value())
        {
            return false;
        }

        const auto editedTileValue = getEditedTile(drawnMap, chosenLayer,
            stroke,
            assignMode);
        const auto pixel = tile::pixelAt(
            editedTileValue,
            getInspectedTileRect(sheetView.getFrameRect(), editedTileValue),
            pointer.pointerOnCanvas);

        if (!pixel.has_value())
        {
            return false;
        }

        if (blockedAsTransitionSlot(viewContext))
        {
            return true;
        }

        if (preferences.paint == map::Paint::Line
            || preferences.paint == map::Paint::Rect
            || preferences.paint == map::Paint::Circle)
        {
            stroke.lineFromCell = pixel;

            return true;
        }

        auto &sheet = atlasSheets.sheet(editedTileValue.atlas);

        viewContext.editSteps.pushUndo();

        if (preferences.paint == map::Paint::Fill)
        {
            tile::paintFill(
                sheet,
                editedTileValue,
                *pixel,
                drawnMap.paletteColors.at(inkPicker.activeInk));
        }
        else
        {
            tile::paint(
                sheet,
                editedTileValue,
                *pixel,
                drawnMap.paletteColors.at(inkPicker.activeInk));
            stroke.brushAtCell = pixel;
            stroke.active = true;
        }

        atlasSheets.touch();

        return true;
    }
    bool AtlasSheetsView::blockedAsTransitionSlot(
        const ViewContext &viewContext)
    {
auto &stroke = viewContext.workbench.stroke;
        auto &drawnMap = viewContext.document.map;

        if (!stroke.selectedTile.has_value()
            || transitionOf(drawnMap.transitions, *stroke.selectedTile)
                   == nullptr)
        {
            return false;
        }

        viewContext.notices.showStatus("this tile is woven from its materials", true);

        return true;
    }

}
