#include <antwika/component/AnimationState.hpp>
#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/AtlasView.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/light/PointLight.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::layoutSidebar(ui::Context &context)
    {
        const auto showPalette =
            activeView == map::View::Atlases
            || activeView == map::View::Character
            || (activeView == map::View::World
                && settings.tool == map::Tool::Lamp);
        const auto showLayers = activeView == map::View::Atlases;
        const auto showExitPanel =
            settings.tool == map::Tool::Exit && activeView == map::View::World;
        const auto showFigures =
            settings.tool == map::Tool::Figure
            && activeView == map::View::World;

        if (showPalette || showLayers || showExitPanel || showFigures)
        {
            if (activeView != map::View::Atlases)
            {
                context.spacer(antwika::ui::kGrowSizing);
            }

            const auto rail = context.column(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::getFixedSize(
                        getRailWidth(
                            viewportRenderer.getWindowSize(),
                            camera::kCanvasSize)),
                    .gap = antwika::editor::kPanelGap
                           * kUiScale,
                    .widgetId = antwika::editor::kRailWidget});

            if (showPalette)
            {
                const auto inks = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding,
                        .widgetId = antwika::editor::
                            kPaletteWidget});

                panelTitle(context, "Palette");

                const auto swatches = context.row(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kFitSizing});

                for (std::size_t which = 0;
                     which < document.map.paletteColors.size();
                     ++which)
                {
                    const auto side =
                        antwika::ui::getFixedSize(
                            kSwatchWidth * kUiScale);
                    const auto swatchBorderPanel = context.panel(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing,
                            .backgroundColor =
                                which == inkPicker.activeInk
                                       ? std::optional{kTextColor}
                                       : std::nullopt,
                            .padding = kUiScale});
                    const auto swatch = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = side,
                            .heightSizing = side,
                            .backgroundColor =
                                document.map.paletteColors.at(which),
                            .padding = 0,
                            .widgetId = tile::getSwatchWidget(which)});
                }

                if (document.map.paletteColors.size() < tile::kMaxInks)
                {
                    const auto side =
                        antwika::ui::getFixedSize(
                            kSwatchWidth * kUiScale);
                    const auto swatchBorderPanel = context.panel(
                        antwika::ui::ContainerSpec{
                            .widthSizing = antwika::ui::kFitSizing,
                            .padding = kUiScale});
                    const auto addInkRow = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing = side,
                            .heightSizing = side,
                            .crossAlignment = antwika::ui::
                                Alignment::Center,
                            .backgroundColor = kGridLineColor,
                            .padding = 0,
                            .widgetId = antwika::editor::
                                kAddInkWidget});

                    context.spacer(antwika::ui::kGrowSizing);
                    context.label("+", kTextColor);
                    context.spacer(antwika::ui::kGrowSizing);
                }
            }

            layoutWorldRail(context);

            if (inkPicker.editingInk.has_value())
            {
                const auto inkPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding});

                panelTitle(context, "Ink");

                context.textField(
                    antwika::ui::TextFieldSpec{
                        .widgetId = decor::
                            kInkHexWidget,
                        .text = inkPicker.hexText,
                        .placeholder = "#rrggbb",
                        .focused = true});
                context.label(
                    "Glow "
                        + std::to_string(
                            glowOf(*inkPicker.editingInk)),
                    kTextColor);
                context.slider(
                    antwika::ui::SliderSpec{
                        .widgetId =
                            antwika::editor::kGlowWidget,
                        .value = glowOf(*inkPicker.editingInk),
                        .range = 100,
                        .dragging =
                            slidingWidget
                            == antwika::editor::kGlowWidget});
                context.button(
                    "Ok",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::
                            kInkOkWidget,
                        .widthSizing = antwika::ui::kGrowSizing});
                context.button(
                    "Cancel",
                    antwika::ui::ButtonSpec{
                        .widgetId = antwika::editor::
                            kInkCancelWidget,
                        .widthSizing = antwika::ui::kGrowSizing});

                if (document.map.paletteColors.size() > 1)
                {
                    context.button(
                        "Delete",
                        antwika::ui::ButtonSpec{
                            .widgetId = antwika::editor::
                                kInkDeleteWidget,
                            .widthSizing = antwika::ui::kGrowSizing});
                }
            }

            layoutFigureChooser(context);

            if (activeView == map::View::Character)
            {
                const auto walking = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding,
                        .widgetId = antwika::editor::
                            kPreviewWidget});
                const auto way = characterView.mark.hoveredWayRow.value_or(
                    characterView.mark.selectedFrame.value_or(0)
                    / character::kCharacterFrames);

                panelTitle(context, getCapitalized(character::getDirectionName(way)));
                context.image(
                    antwika::ui::Icon{
                        .sheetTexture = characterView.getTexture(),
                        .sourceRect = character::getCharacterCell(
                            way,
                            character::getCurrentFrame(
                                component::AnimationState{
                                    .direction = static_cast<
                                        std::uint8_t>(
                                        way),
                                    .walking = true,
                                    .startedAtTick = 0},
                                tick)
                                % character::kCharacterFrames),
                        .scale =
                            kUiScale * kPreviewScale},
                    kWhiteColor);
            }

            if (showLayers)
            {
                const auto layersPanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding,
                        .widgetId = antwika::editor::
                            kLayersPanelWidget});

                {
                    const auto heading = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing =
                                antwika::ui::kGrowSizing,
                            .crossAlignment = antwika::ui::
                                Alignment::Center,
                            .backgroundColor = kTitleBarColor,
                            .padding = kPanelPadding});

                    context.label("Layers", kTextColor);
                    context.spacer(antwika::ui::kGrowSizing);
                    context.button(
                        "+",
                        antwika::ui::ButtonSpec{
                            .widgetId = map::
                                kAddLayerWidget});
                    context.button(
                        "x",
                        antwika::ui::ButtonSpec{
                            .widgetId = map::
                                kRemoveLayerWidget});
                }

                for (std::size_t layerIndex = 0;
                     layerIndex < document.map.layers.size();
                     ++layerIndex)
                {
                    const auto reversedIndex =
                        document.map.layers.size() - 1 - layerIndex;

                    context.button(
                        map::getLayerLabel(reversedIndex),
                        antwika::ui::ButtonSpec{
                            .widgetId = map::getLayerWidget(reversedIndex),
                            .widthSizing = antwika::ui::kGrowSizing,
                            .fillColor = reversedIndex == chosenLayer
                                       ? kSelectionAccentColor
                                       : kGridLineColor});
                }
            }

            if (showLayers)
            {
                layoutDecorRail(context);
            }

            layoutVariantRail(context);
            layoutFlipRail(context);
            layoutTransitionRail(context);

            if (showLayers && selectedTile.has_value())
            {
                const auto tilePanel = context.column(
                    antwika::ui::ContainerSpec{
                        .widthSizing = antwika::ui::kGrowSizing,
                        .backgroundColor = kPanelColor,
                        .padding = kPanelPadding,
                        .widgetId = decor::
                            kTilingPanelWidget});

                {
                    const auto heading = context.row(
                        antwika::ui::ContainerSpec{
                            .widthSizing =
                                antwika::ui::kGrowSizing,
                            .crossAlignment = antwika::ui::
                                Alignment::Center,
                            .backgroundColor = kTitleBarColor,
                            .padding = kPanelPadding});

                    context.label("Tiling", kTextColor);
                    context.button(
                        "*",
                        antwika::ui::ButtonSpec{
                            .widgetId = decor::
                                kRerollPreviewWidget});
                    context.spacer(antwika::ui::kGrowSizing);
                    context.checkbox(
                        "Auto",
                        antwika::ui::CheckboxSpec{
                            .widgetId = decor::
                                kAutoPreviewWidget,
                            .checked = previewAuto});
                }

                if (previewTiles.has_value())
                {
                    for (std::size_t row = 0; row < 3;
                         ++row)
                    {
                        const auto rowWidget = context.row(
                            antwika::ui::ContainerSpec{
                                .widthSizing =
                                    antwika::ui::kFitSizing});

                        for (std::size_t column = 0;
                             column < 3;
                             ++column)
                        {
                            const auto previewTile =
                                previewTiles->at(
                                    (row * 3) + column);

                            if (!previewTile.has_value())
                            {
                                const auto width =
                                    antwika::ui::
                                        getFixedSize(
                                            tilemap::kFloorTileSize
                                                .width
                                            * kUiScale);
                                const auto height =
                                    antwika::ui::
                                        getFixedSize(
                                            tilemap::kFloorTileSize
                                                .height
                                            * kUiScale);
                                const auto bare =
                                    context.panel(
                                        antwika::ui::ContainerSpec{
                                            .widthSizing =
                                                width,
                                            .heightSizing =
                                                height,
                                            .backgroundColor =
                                                kGridLineColor,
                                            .padding =
                                                0});

                                continue;
                            }

                            const auto tile = *previewTile;
                            const auto sourceRect =
                                tilemap::getTileSource(tile);
                            const antwika::gfx::Rect
                                cutRect{
                                    .originPoint =
                                        {.x = static_cast<
                                             std::int32_t>(
                                             sourceRect.originPoint
                                                 .x),
                                         .y = static_cast<
                                             std::int32_t>(
                                             sourceRect.originPoint
                                                 .y)},
                                    .size = tilemap::tileSizeOf(
                                        tile.atlas)};

                            context.image(
                                antwika::ui::Icon{
                                    .sheetTexture =
                                        atlasSheets.getTexture(
                                            tile.atlas),
                                    .sourceRect = cutRect,
                                    .scale = kUiScale},
                                kWhiteColor);
                        }
                    }
                }
                else
                {
                    context.label("No way to lay", kGridLineColor);
                }
            }
        }
    }

}
