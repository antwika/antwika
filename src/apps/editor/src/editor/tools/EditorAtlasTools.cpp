#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/tilemap/TileEdges.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/ui/DoubleClick.hpp>
#include <antwika/voxel/VoxelCube.hpp>
#include <antwika/voxelmap/VoxelPick.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::duplicateTile(
        const geometry::GridCell fromCell,
        const geometry::GridCell toCell)
    {
        const auto source =
            document.map.tilemap.getEntryAt(fromCell.column, fromCell.row);
        auto target = document.map.tilemap.getEntryAt(toCell.column, toCell.row);

        if (!target.has_value())
        {
            target = tilemap::suggestedTileFor(document.map.tilemap, toCell);

            if (target.has_value())
            {
                tilemap::putTile(document.map.tilemap, toCell, *target);
            }
        }

        if (!source.has_value() || !target.has_value()
            || source->atlas != target->atlas
            || *source == *target
            || transitionOf(document.map.transitions, *target)
                   != nullptr)
        {
            return;
        }

        copyTilePixels(*source, *target);

        atlasSheets.touch();
        rebuildWorld();
    }

    void Editor::onScrolled(const input::PointerScrolled &rolledScrolled)
    {
        if (rolledScrolled.vertical != 0
            && pointer.hoveredWidget
                   == decor::kVariantWeightWidget
            && selectedTile.has_value())
        {
            if (tick >= lastWheelNudgeTick + 60)
            {
                pushUndo();
            }

            const auto weight = variantWeightOf(*selectedTile);
            const auto nextIndex =
                rolledScrolled.vertical > 0
                    ? std::min<int>(
                          weight + 1,
                          decor::kFullFrequency)
                    : std::max<int>(weight - 1, 0);

            document.map.familyGroups = getWithVariantWeightSet(
                document.map.familyGroups,
                *selectedTile,
                static_cast<std::uint8_t>(nextIndex));
            lastWheelNudgeTick = tick;
            remeshAfterNudge = true;

            return;
        }

        if (rolledScrolled.vertical != 0
            && pointer.hoveredWidget == decor::kFrequencyWidget
            && isDecorLayer() && selectedTile.has_value())
        {
            if (tick >= lastWheelNudgeTick + 60)
            {
                pushUndo();
            }

            ensureDecor();

            const auto *nudgedDecor = decor::decorOf(
                document.map.decor, *selectedTile);
            const auto nextIndex =
                rolledScrolled.vertical > 0
                    ? std::min<int>(
                          nudgedDecor->frequency + 1,
                          decor::kFullFrequency)
                    : std::max<int>(nudgedDecor->frequency - 1, 0);

            document.map.decor = getWithFrequencySet(
                document.map.decor,
                *selectedTile,
                static_cast<std::uint8_t>(nextIndex));
            lastWheelNudgeTick = tick;
            remeshAfterNudge = true;

            return;
        }

        if (rolledScrolled.vertical != 0
            && pointer.hoveredWidget
                   == decor::kDecorWeightWidget
            && isDecorLayer() && selectedTile.has_value())
        {
            if (tick >= lastWheelNudgeTick + 60)
            {
                pushUndo();
            }

            ensureDecor();

            const auto *nudgedDecor = decor::decorOf(
                document.map.decor, *selectedTile);
            const auto nextIndex =
                rolledScrolled.vertical > 0
                    ? std::min<int>(
                          nudgedDecor->weight + 1,
                          decor::kFullFrequency)
                    : std::max<int>(nudgedDecor->weight - 1, 0);

            document.map.decor = getWithWeightSet(
                document.map.decor,
                *selectedTile,
                static_cast<std::uint8_t>(nextIndex));
            lastWheelNudgeTick = tick;
            remeshAfterNudge = true;

            return;
        }

        if (rolledScrolled.vertical != 0
            && activeView == map::View::Atlases)
        {
            gridZoom = std::clamp(
                gridZoom
                    * (rolledScrolled.vertical > 0
                           ? kGridZoomStep
                           : 1.0F / kGridZoomStep),
                kMinGridZoom,
                kMaxGridZoom);
        }
        else if (rolledScrolled.vertical != 0)
        {
            auto &zoomValue = play.playing ? play.game->zoom(
                ) : cameraRig.view.zoom;

            zoomValue = std::clamp(
                zoomValue
                    + (rolledScrolled.vertical > 0
                           ? camera::kZoomStep
                           : -camera::kZoomStep),
                camera::kMinZoom,
                camera::kMaxZoom);
        }

        return;
    }

    bool Editor::beginSliderDrag(
        const ui::Interactions &interactions)
    {
        if (interactions.slidChange.has_value()
            && interactions.slidChange->sliderWidget
                   == decor::kFrequencyWidget
            && selectedTile.has_value())
        {
            pushUndo();
            ensureDecor();
            document.map.decor = getWithFrequencySet(
                document.map.decor,
                *selectedTile,
                static_cast<std::uint8_t>(
                    interactions.slidChange->value));
            slidingWidget = decor::kFrequencyWidget;

            return true;
        }

        if (interactions.slidChange.has_value()
            && interactions.slidChange->sliderWidget
                   == decor::kDecorWeightWidget
            && selectedTile.has_value())
        {
            pushUndo();
            ensureDecor();
            document.map.decor = getWithWeightSet(
                document.map.decor,
                *selectedTile,
                static_cast<std::uint8_t>(
                    interactions.slidChange->value));
            slidingWidget = decor::kDecorWeightWidget;

            return true;
        }

        if (interactions.slidChange.has_value()
            && interactions.slidChange->sliderWidget
                   == decor::kVariantWeightWidget
            && selectedTile.has_value())
        {
            pushUndo();
            document.map.familyGroups = getWithVariantWeightSet(
                document.map.familyGroups,
                *selectedTile,
                static_cast<std::uint8_t>(
                    interactions.slidChange->value));
            slidingWidget = decor::kVariantWeightWidget;

            return true;
        }

        if (interactions.slidChange.has_value()
            && interactions.slidChange->sliderWidget
                   == antwika::editor::kGlowWidget
            && inkPicker.editingInk.has_value())
        {
            if (*inkPicker.editingInk < document.map.glows.size())
            {
                document.map.glows.at(*inkPicker.editingInk) =
                    static_cast<std::uint8_t>(
                        interactions.slidChange->value);
            }

            slidingWidget = antwika::editor::kGlowWidget;

            return true;
        }

        if (interactions.slidChange.has_value()
            && interactions.slidChange->sliderWidget
                   == antwika::editor::kAmbientWidget)
        {
            pushUndo();
            document.map.ambient = static_cast<std::uint8_t>(
                interactions.slidChange->value);
            slidingWidget = antwika::editor::kAmbientWidget;

            return true;
        }

        return false;
    }

    bool Editor::consumePickerPress(
        const input::PointerButtonPressed &downPressed)
    {
        if (!inkPicker.editingInk.has_value())
        {
            return false;
        }


            const auto projectToScreen =
                viewportRenderer.getViewport().toCanvas(
                    antwika::gfx::Point{
                        .x = downPressed.position.x,
                        .y = downPressed.position.y});

            pointer.pointerOnCanvas = antwika::gfx::PointF{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};

            const auto withLeft =
                downPressed.button == input::MouseButton::Left;
            const auto takenColor =
                withLeft ? getColorAtPoint(
                               camera::kCanvasSize, inkPicker.pickerHsv,
                                   pointer.pointerOnCanvas)
                         : std::nullopt;

            if (takenColor.has_value())
            {
                if (!inkPicker.pickerDragging)
                {
                    pushUndo();
                }

                inkPicker.pickerHsv = *takenColor;
                recolorInk(colorOf(inkPicker.pickerHsv));
                inkPicker.hexText = getColorToHex(
                    document.map.paletteColors.at(*inkPicker.editingInk));
                inkPicker.pickerDragging = true;

                return true;
            }

            if (isOnPicker(camera::kCanvasSize, pointer.pointerOnCanvas))
            {
                return true;
            }

            if (withLeft)
            {
                inkPicker.editingInk.reset();

                return true;
            }

        return false;
    }

    void Editor::endSliderDrag()
    {
        if (slidingWidget == decor::kFrequencyWidget
            || slidingWidget == decor::kDecorWeightWidget
            || slidingWidget
                   == decor::kVariantWeightWidget)
        {
            rebuildWorld();
        }

        if (slidingWidget == antwika::editor::kGlowWidget)
        {
            atlasSheets.touch();
        }

        slidingWidget.reset();
    }

    bool Editor::consumePaletteWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        for (std::size_t which = 0;
             which < document.map.paletteColors.size();
             ++which)
        {
            if (interactions.activatedWidget
                != tile::getSwatchWidget(which))
            {
                continue;
            }

            consumedKey = true;

            if (ui::isDoubleClick(
                    pointer.clickTracker, tick, pointer.pointerOnCanvas))
            {
                inkPicker.editingInk = which;
                inkPicker.inkBeforeEditColor =
                    document.map.paletteColors.at(which);
                inkPicker.glowBeforeEdit = glowOf(which);
                inkPicker.pickerHsv = hsvOf(
                    document.map.paletteColors.at(which));
                inkPicker.hexText = getColorToHex(
                    document.map.paletteColors.at(which));
                carryInk();
            }

            pointer.clickTracker =
                ui::getTrackClick(tick, pointer.pointerOnCanvas);
            inkPicker.activeInk = which;
        }

        if (interactions.activatedWidget
                == antwika::editor::
                    kAddInkWidget
            && document.map.paletteColors.size() < tile::kMaxInks)
        {
            pushUndo();
            document.map.paletteColors.push_back(
                document.map.paletteColors.at(inkPicker.activeInk));
            document.map.glows.push_back(
                inkPicker.activeInk < document.map.glows.size()
                    ? document.map.glows.at(inkPicker.activeInk)
                    : 0);
            inkPicker.activeInk = document.map.paletteColors.size() - 1;
            inkPicker.editingInk = inkPicker.activeInk;
            inkPicker.inkBeforeEditColor = document.map.paletteColors.at(
                inkPicker.activeInk);
            inkPicker.glowBeforeEdit = glowOf(inkPicker.activeInk);
            inkPicker.pickerHsv =
                hsvOf(document.map.paletteColors.at(inkPicker.activeInk));
            inkPicker.hexText =
                getColorToHex(document.map.paletteColors.at(inkPicker.activeInk));
            carryInk();
            consumedKey = true;
        }

        if (interactions.activatedWidget
            == antwika::editor::kInkOkWidget)
        {
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::
                    kInkCancelWidget
            && inkPicker.editingInk.has_value())
        {
            recolorInk(inkPicker.inkBeforeEditColor);
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget
                == antwika::editor::
                    kInkDeleteWidget
            && inkPicker.editingInk.has_value()
            && document.map.paletteColors.size() > 1)
        {
            pushUndo();
            document.map.paletteColors.erase(
                std::next(
                    document.map.paletteColors.begin(),
                    static_cast<
                        std::ptrdiff_t>(
                        *inkPicker.editingInk)));

            if (*inkPicker.editingInk < document.map.glows.size())
            {
                document.map.glows.erase(
                    std::next(
                        document.map.glows.begin(),
                        static_cast<
                            std::ptrdiff_t>(
                            *inkPicker.editingInk)));
            }
            inkPicker.activeInk = std::min(
                inkPicker.activeInk,
                document.map.paletteColors.size() - 1);
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        return consumedKey;
    }

    std::uint8_t Editor::glowOf(const std::size_t ink) const
    {
        return ink < document.map.glows.size() ? document.map.glows.at(ink) : 0;
    }

    void Editor::carryInk()
    {
        inkPicker.carriedInk = {};
        inkPicker.carriedCharacterInk.clear();
        inkPicker.carriedFigureInk.clear();

        if (!tile::isSoleInk(document.map.paletteColors, *inkPicker.editingInk))
        {
            return;
        }

        const auto color = document.map.paletteColors.at(*inkPicker.editingInk);

        for (std::size_t sheet = 0;
             sheet < atlasSheets.getSheets().size();
             ++sheet)
        {
            inkPicker.carriedInk.at(sheet) =
                tile::getPaintedWith(atlasSheets.sheet(sheet), color);
        }

        if (*inkPicker.editingInk == character::kTransparentInk)
        {
            return;
        }

        inkPicker.carriedCharacterInk =
            tile::getPaintedWith(characterView.getSheet(), color);
        for (const auto &skin : characterView.getSkins())
        {
            inkPicker.carriedFigureInk.push_back(
                tile::getPaintedWith(skin, color));
        }
    }

    void Editor::recolorInk(const gfx::Color nextColor)
    {
        document.map.paletteColors.at(*inkPicker.editingInk) = nextColor;

        for (std::size_t sheet = 0;
             sheet < atlasSheets.getSheets().size();
             ++sheet)
        {
            if (!inkPicker.carriedInk.at(sheet).empty())
            {
                tile::repaintAt(
                    atlasSheets.sheet(sheet),
                    inkPicker.carriedInk.at(sheet),
                    nextColor);
                atlasSheets.touch();
            }
        }

        if (!inkPicker.carriedCharacterInk.empty())
        {
            tile::repaintAt(
                characterView.getSheet(),
                inkPicker.carriedCharacterInk,
                nextColor);
            characterView.touch();
        }

        for (std::size_t figure = 0;
             figure < inkPicker.carriedFigureInk.size()
             && figure < characterView.getSkins().size();
             ++figure)
        {
            if (inkPicker.carriedFigureInk.at(figure).empty())
            {
                continue;
            }

            auto paintedSkin = characterView.getSkins().at(figure);

            tile::repaintAt(
                paintedSkin,
                inkPicker.carriedFigureInk.at(figure),
                nextColor);
            characterView.repaint(
                viewportRenderer, figure, std::move(paintedSkin));
        }
    }

    bool Editor::mayAdjoin(
        const tilemap::Tile oneTile, const tilemap::Tile otherTile)
    {
        const auto &rules =
            isDecorLayer() ? document.map.decorRules : worldMeshes.getRules();

        for (const auto edge : tilemap::kEveryTileEdge)
        {
            if (decor::tilesCompatible(rules, oneTile, edge, otherTile)
                && decor::tilesCompatible(rules, otherTile, voxel::getFacing(edge),
                    oneTile))
            {
                return true;
            }
        }

        return false;
    }

}
