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
            map.tilemap.at(fromCell.column, fromCell.row);
        auto target = map.tilemap.at(toCell.column, toCell.row);

        if (!target.has_value())
        {
            target = tilemap::suggestedTileFor(map.tilemap, toCell);

            if (target.has_value())
            {
                tilemap::putTile(map.tilemap, toCell, *target);
            }
        }

        if (!source.has_value() || !target.has_value()
            || source->atlas != target->atlas
            || *source == *target
            || transitionOf(map.transitions, *target)
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

            map.familyGroups = withVariantWeightSet(
                map.familyGroups,
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
                map.decor, *selectedTile);
            const auto nextIndex =
                rolledScrolled.vertical > 0
                    ? std::min<int>(
                          nudgedDecor->frequency + 1,
                          decor::kFullFrequency)
                    : std::max<int>(nudgedDecor->frequency - 1, 0);

            map.decor = withFrequencySet(
                map.decor,
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
                map.decor, *selectedTile);
            const auto nextIndex =
                rolledScrolled.vertical > 0
                    ? std::min<int>(
                          nudgedDecor->weight + 1,
                          decor::kFullFrequency)
                    : std::max<int>(nudgedDecor->weight - 1, 0);

            map.decor = withWeightSet(
                map.decor,
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
            auto &zoomValue = playing ? game->zoom() : cameraView.zoom;

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
            map.decor = withFrequencySet(
                map.decor,
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
            map.decor = withWeightSet(
                map.decor,
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
            map.familyGroups = withVariantWeightSet(
                map.familyGroups,
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
            if (*inkPicker.editingInk < map.glows.size())
            {
                map.glows.at(*inkPicker.editingInk) =
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
            map.ambient = static_cast<std::uint8_t>(
                interactions.slidChange->value);
            slidingWidget = antwika::editor::kAmbientWidget;

            return true;
        }

        return false;
    }

    bool Editor::handlePickerPress(
        const input::PointerButtonPressed &downPressed)
    {
        if (!inkPicker.editingInk.has_value())
        {
            return false;
        }


            const auto projectToScreen =
                viewportRenderer.viewport().toCanvas(
                    antwika::gfx::Point{
                        .x = downPressed.position.x,
                        .y = downPressed.position.y});

            pointer.pointerOnCanvas = antwika::gfx::PointF{
                static_cast<float>(projectToScreen.x),
                static_cast<float>(projectToScreen.y)};

            const auto withLeft =
                downPressed.button == input::MouseButton::Left;
            const auto takenColor =
                withLeft ? colorAtPoint(
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
                inkPicker.hexText = colorToHex(
                    map.paletteColors.at(*inkPicker.editingInk));
                inkPicker.pickerDragging = true;

                return true;
            }

            if (onPicker(camera::kCanvasSize, pointer.pointerOnCanvas))
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

    bool Editor::handlePaletteWidgets(
        const ui::Interactions &interactions)
    {
        auto consumedKey = false;

        for (std::size_t which = 0;
             which < map.paletteColors.size();
             ++which)
        {
            if (interactions.activatedWidget
                != tile::swatchWidget(which))
            {
                continue;
            }

            consumedKey = true;

            if (ui::isDoubleClick(
                    pointer.clickTracker, tick, pointer.pointerOnCanvas))
            {
                inkPicker.editingInk = which;
                inkPicker.inkBeforeEditColor =
                    map.paletteColors.at(which);
                inkPicker.glowBeforeEdit = glowOf(which);
                inkPicker.pickerHsv = hsvOf(
                    map.paletteColors.at(which));
                inkPicker.hexText = colorToHex(
                    map.paletteColors.at(which));
                carryInk();
            }

            pointer.clickTracker =
                ui::trackClick(tick, pointer.pointerOnCanvas);
            inkPicker.activeInk = which;
        }

        if (interactions.activatedWidget
                == antwika::editor::
                    kAddInkWidget
            && map.paletteColors.size() < tile::kMaxInks)
        {
            pushUndo();
            map.paletteColors.push_back(
                map.paletteColors.at(inkPicker.activeInk));
            map.glows.push_back(
                inkPicker.activeInk < map.glows.size()
                    ? map.glows.at(inkPicker.activeInk)
                    : 0);
            inkPicker.activeInk = map.paletteColors.size() - 1;
            inkPicker.editingInk = inkPicker.activeInk;
            inkPicker.inkBeforeEditColor = map.paletteColors.at(
                inkPicker.activeInk);
            inkPicker.glowBeforeEdit = glowOf(inkPicker.activeInk);
            inkPicker.pickerHsv =
                hsvOf(map.paletteColors.at(inkPicker.activeInk));
            inkPicker.hexText =
                colorToHex(map.paletteColors.at(inkPicker.activeInk));
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
            && map.paletteColors.size() > 1)
        {
            pushUndo();
            map.paletteColors.erase(
                std::next(
                    map.paletteColors.begin(),
                    static_cast<
                        std::ptrdiff_t>(
                        *inkPicker.editingInk)));

            if (*inkPicker.editingInk < map.glows.size())
            {
                map.glows.erase(
                    std::next(
                        map.glows.begin(),
                        static_cast<
                            std::ptrdiff_t>(
                            *inkPicker.editingInk)));
            }
            inkPicker.activeInk = std::min(
                inkPicker.activeInk,
                map.paletteColors.size() - 1);
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        return consumedKey;
    }

    std::uint8_t Editor::glowOf(const std::size_t ink) const
    {
        return ink < map.glows.size() ? map.glows.at(ink) : 0;
    }

    void Editor::carryInk()
    {
        inkPicker.carriedInk = {};
        inkPicker.carriedCharacterInk.clear();
        inkPicker.carriedFigureInk.clear();

        if (!tile::soleInk(map.paletteColors, *inkPicker.editingInk))
        {
            return;
        }

        const auto color = map.paletteColors.at(*inkPicker.editingInk);

        for (std::size_t sheet = 0;
             sheet < atlasSheets.sheets().size();
             ++sheet)
        {
            inkPicker.carriedInk.at(sheet) =
                tile::paintedWith(atlasSheets.sheet(sheet), color);
        }

        if (*inkPicker.editingInk == character::kTransparentInk)
        {
            return;
        }

        inkPicker.carriedCharacterInk =
            tile::paintedWith(characterView.sheet(), color);
        for (const auto &skin : characterView.skins())
        {
            inkPicker.carriedFigureInk.push_back(
                tile::paintedWith(skin, color));
        }
    }

    void Editor::recolorInk(const gfx::Color nextColor)
    {
        map.paletteColors.at(*inkPicker.editingInk) = nextColor;

        for (std::size_t sheet = 0;
             sheet < atlasSheets.sheets().size();
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
                characterView.sheet(),
                inkPicker.carriedCharacterInk,
                nextColor);
            characterView.touch();
        }

        for (std::size_t figure = 0;
             figure < inkPicker.carriedFigureInk.size()
             && figure < characterView.skins().size();
             ++figure)
        {
            if (inkPicker.carriedFigureInk.at(figure).empty())
            {
                continue;
            }

            auto paintedSkin = characterView.skins().at(figure);

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
            isDecorLayer() ? map.decorRules : worldMeshes.rules();

        for (const auto edge : tilemap::kEveryTileEdge)
        {
            if (decor::tilesCompatible(rules, oneTile, edge, otherTile)
                && decor::tilesCompatible(rules, otherTile, voxel::facing(edge),
                    oneTile))
            {
                return true;
            }
        }

        return false;
    }

}
