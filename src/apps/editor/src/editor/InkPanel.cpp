#include "antwika/editor/editor/InkPanel.hpp"

#include <antwika/camera/FlyCamera.hpp>
#include <antwika/character/Character.hpp>
#include <antwika/gfx/PointF.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/tile/TilePaint.hpp>
#include <antwika/ui/DoubleClick.hpp>

#include "antwika/editor/ui/ColorPicker.hpp"
#include "antwika/editor/ui/ToolPanel.hpp"

#include "antwika/editor/ui/WidgetIds.hpp"

namespace antwika::editor
{

    InkPanel::InkPanel(
        EditorDocument &document,
        render::AtlasSheets &atlasSheets,
        CharacterSheetView &characterView,
        render::CharacterSkins &characterSkins,
        gfx::ViewportRenderer &viewportRenderer,
        IEditSteps &editSteps) noexcept
        : document(document),
          atlasSheets(atlasSheets),
          characterView(characterView),
          characterSkins(characterSkins),
          viewportRenderer(viewportRenderer),
          editSteps(editSteps)
    {
    }

    std::uint8_t InkPanel::glowOf(const std::size_t ink) const
    {
        return ink < document.map.glows.size() ? document.map.glows.at(ink) : 0;
    }

    void InkPanel::carryInk()
    {
        inkPicker.carriedInk = {};
        inkPicker.carriedCharacterInk.clear();
        inkPicker.carriedSkinInk.clear();

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
        for (const auto &skin : characterSkins.getSheets())
        {
            inkPicker.carriedSkinInk.push_back(
                tile::getPaintedWith(skin, color));
        }
    }

    void InkPanel::recolorInk(const gfx::Color wantedColor)
    {
        const auto nextColor = tile::soleInkColorOf(
            document.map.paletteColors, *inkPicker.editingInk, wantedColor);

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

        for (std::size_t skin = 0;
             skin < inkPicker.carriedSkinInk.size()
             && skin < characterSkins.getSheets().size();
             ++skin)
        {
            if (inkPicker.carriedSkinInk.at(skin).empty())
            {
                continue;
            }

            auto paintedSkin = characterSkins.getSheets().at(skin);

            tile::repaintAt(
                paintedSkin,
                inkPicker.carriedSkinInk.at(skin),
                nextColor);
            characterView.repaint(
                viewportRenderer, characterSkins, skin, std::move(paintedSkin));
        }
    }

    bool InkPanel::consumePaletteWidgets(
        const ui::Interactions &interactions,
        PointerTrack &pointer,
        const std::uint32_t tick)
    {
        auto consumedKey = false;

        for (std::size_t which = 0;
             which < document.map.paletteColors.size();
             ++which)
        {
            if (interactions.activatedWidget
                != getSwatchWidget(which))
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

        if (interactions.activatedWidget == kAddInkWidget
            && document.map.paletteColors.size() < tile::kMaxInks)
        {
            editSteps.pushUndo();
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

        if (interactions.activatedWidget == kInkOkWidget)
        {
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget == kInkCancelWidget
            && inkPicker.editingInk.has_value())
        {
            recolorInk(inkPicker.inkBeforeEditColor);
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        if (interactions.activatedWidget == kInkDeleteWidget
            && inkPicker.editingInk.has_value()
            && document.map.paletteColors.size() > 1)
        {
            editSteps.pushUndo();
            document.map.paletteColors.erase(
                std::next(
                    document.map.paletteColors.begin(),
                    static_cast<std::ptrdiff_t>(*inkPicker.editingInk)));

            if (*inkPicker.editingInk < document.map.glows.size())
            {
                document.map.glows.erase(
                    std::next(
                        document.map.glows.begin(),
                        static_cast<std::ptrdiff_t>(*inkPicker.editingInk)));
            }
            inkPicker.activeInk = std::min(
                inkPicker.activeInk,
                document.map.paletteColors.size() - 1);
            inkPicker.editingInk.reset();
            consumedKey = true;
        }

        return consumedKey;
    }

    bool InkPanel::consumePickerPress(
        const input::PointerButtonPressed &downPressed,
        PointerTrack &pointer,
        const float railWidth)
    {
        if (!inkPicker.editingInk.has_value())
        {
            return false;
        }

        const auto projectToScreen =
            viewportRenderer.getViewport().toCanvas(
                gfx::Point{
                    .x = downPressed.position.x,
                    .y = downPressed.position.y});

        pointer.pointerOnCanvas = gfx::PointF{
            static_cast<float>(projectToScreen.x),
            static_cast<float>(projectToScreen.y)};

        const auto withLeft =
            downPressed.button == input::MouseButton::Left;
        const auto takenColor =
            withLeft ? getColorAtPoint(
                           camera::kCanvasSize,
                           railWidth,
                           inkPicker.pickerHsv,
                           pointer.pointerOnCanvas)
                     : std::nullopt;

        if (takenColor.has_value())
        {
            if (!inkPicker.pickerDragging)
            {
                editSteps.pushUndo();
            }

            inkPicker.pickerHsv = *takenColor;
            recolorInk(colorOf(inkPicker.pickerHsv));
            inkPicker.hexText = getColorToHex(
                document.map.paletteColors.at(*inkPicker.editingInk));
            inkPicker.pickerDragging = true;

            return true;
        }

        if (isOnPicker(
                camera::kCanvasSize, railWidth, pointer.pointerOnCanvas))
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

}
