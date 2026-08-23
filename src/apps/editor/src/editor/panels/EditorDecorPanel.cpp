#include <antwika/decor/Decor.hpp>
#include <antwika/editor/ui/EditorLook.hpp>
#include <antwika/geometry/Grid.hpp>
#include <antwika/gfx/Color.hpp>
#include <antwika/map/Layers.hpp>
#include <antwika/tilemap/Tilemap.hpp>
#include <antwika/tile/TilePaint.hpp>

#include "antwika/editor/Editor.hpp"

namespace antwika::editor
{

    void Editor::ensureDecor()
    {
        if (isDecorLayer() && selectedTile.has_value()
            && decor::decorOf(document.map.decor, *selectedTile)
                   == nullptr)
        {
            document.map.decor = decor::withDecorToggled(
                document.map.decor, *selectedTile, chosenLayer);
        }
    }

    tilemap::Tile Editor::editedTile()
    {
        if (!isDecorLayer() || !selectedTile.has_value()
            || assignMode.framePicked == 0)
        {
            return *selectedTile;
        }

        const auto *decor =
            decor::decorOf(document.map.decor, *selectedTile);

        return decor != nullptr
                       && assignMode.framePicked
                              < decor->frameTiles.size()
                   ? decor->frameTiles.at(assignMode.framePicked)
                   : *selectedTile;
    } // GCOVR_EXCL_LINE

    std::optional<tilemap::Tile> Editor::freeTileSlot(
        const tilemap::Atlas atlas)
    {
        std::set<tilemap::Tile> takenTiles;

        for (const auto &decor : document.map.decor)
        {
            for (const auto frame : decor.frameTiles)
            {
                takenTiles.insert(frame);
            }
        }

        for (const auto &flip : document.map.flipAnimations)
        {
            for (const auto frame : flip.frameTiles)
            {
                takenTiles.insert(frame);
            }
        }

        for (const auto &transition : document.map.transitions)
        {
            takenTiles.insert(transition.maskTile);
            takenTiles.insert(transition.outputTile);
        }

        for (std::uint16_t index = 0;
             index < tilemap::kAtlasColumns * tilemap::kAtlasRows;
             ++index)
        {
            const tilemap::Tile tile{.atlas = atlas, .index = index};

            if (!tilemap::cellHoldingTile(document.map.tilemap,
                    tile).has_value()
                && !takenTiles.contains(tile))
            {
                return tile;
            }
        }

        return std::nullopt;
    } // GCOVR_EXCL_LINE

    void Editor::copyTilePixels(
        const tilemap::Tile fromTile, const tilemap::Tile toTile)
    {
        auto &sheet =
            atlasSheets.sheet(fromTile.atlas);
        const auto size = tilemap::tileSizeOf(fromTile.atlas);

        for (std::uint32_t row = 0; row < size.height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < size.width;
                 ++column)
            {
                const geometry::GridCell pixelCell{
                    .column = column, .row = row};

                tile::paint(
                    sheet,
                    toTile,
                    pixelCell,
                    tile::paintedAt(sheet, fromTile, pixelCell));
            }
        }
    }

    void Editor::wipeTile(const tilemap::Tile tile)
    {
        auto &sheet =
            atlasSheets.sheet(tile.atlas);
        const auto size = tilemap::tileSizeOf(tile.atlas);

        for (std::uint32_t row = 0; row < size.height; ++row)
        {
            for (std::uint32_t column = 0;
                 column < size.width;
                 ++column)
            {
                tile::paint(
                    sheet,
                    tile,
                    geometry::GridCell{
                        .column = column, .row = row},
                    antwika::gfx::Color{.alpha = 0});
            }
        }

        atlasSheets.touch();
    }

    void Editor::layoutDecorRail(ui::Context &context)
    {
        if (!selectedTile.has_value() || !isDecorLayer())
        {
            return;
        }

        const auto *decor =
            decor::decorOf(document.map.decor, *selectedTile);
        const auto decorShown =
            decor != nullptr
                   ? *decor
                   : decor::DecorTile{
                    .tile = *selectedTile,
                    .frameTiles = {*selectedTile},
                    .layer = chosenLayer,
                    .spanTiles = {*selectedTile}};
        const auto decorPanel = context.column(
            antwika::ui::ContainerSpec{
                .widthSizing = antwika::ui::kGrowSizing,
                .backgroundColor = kPanelColor,
                .padding = kPanelPadding});

        panelTitle(context, "Decor");
        context.label(
            "on " + map::layerLabel(decorShown.layer),
            decorShown.layer == chosenLayer ? kGridLineColor
                              : kSelectionAccentColor);

        if (decor != nullptr && decorShown.layer != chosenLayer)
        {
            context.button(
                "move to " + map::layerLabel(chosenLayer),
                antwika::ui::ButtonSpec{
                    .widgetId = decor::kDecorMoveWidget,
                    .widthSizing = antwika::ui::kGrowSizing});
        }

        context.checkbox(
            "pick bases",
            antwika::ui::CheckboxSpec{
                .widgetId = decor::kPickBaseTilesWidget,
                .checked = assignMode.basePicking});
        context.label(
            "frequency " + std::to_string(decorShown.frequency),
            kTextColor);
        context.slider(
            antwika::ui::SliderSpec{
                .widgetId = decor::kFrequencyWidget,
                .value = decorShown.frequency,
                .range = decor::kFullFrequency,
                .dragging =
                    slidingWidget
                    == decor::kFrequencyWidget});
        context.label(
            "weight " + std::to_string(decorShown.weight),
            kTextColor);
        context.slider(
            antwika::ui::SliderSpec{
                .widgetId = decor::kDecorWeightWidget,
                .value = decorShown.weight,
                .range = decor::kFullFrequency,
                .dragging =
                    slidingWidget
                    == decor::kDecorWeightWidget});
        layoutSpanRows(context, decorShown);

        if (decor::decorSpanned(decorShown))
        {
            return;
        }

        context.label(
            "frames "
                + std::to_string(decorShown.frameTiles.size()),
            kTextColor);

        {
            const auto frames = context.row(
                antwika::ui::ContainerSpec{
                    .widthSizing = antwika::ui::kGrowSizing});

            for (std::size_t frame = 0;
                 frame < decorShown.frameTiles.size();
                 ++frame)
            {
                context.button(
                    std::to_string(frame + 1),
                    antwika::ui::ButtonSpec{
                        .widgetId = decor::frameWidget(
                            frame),
                        .fillColor =
                            frame == assignMode.framePicked
                                   ? kSelectionAccentColor
                                   : kGridLineColor});
            }

            if (decorShown.frameTiles.size()
                < decor::kMaxDecorFrames)
            {
                context.button(
                    "+",
                    antwika::ui::ButtonSpec{
                        .widgetId = decor::
                            kFrameAddWidget});
            }
        }

        const auto strolling = decor::decorFrameAt(decorShown, tick);
        const auto shownFrom = tilemap::tileSource(strolling);
        const antwika::gfx::Rect walkCutRect{
            .originPoint =
                {.x = static_cast<std::int32_t>(
                     shownFrom.originPoint.x),
                 .y = static_cast<std::int32_t>(
                     shownFrom.originPoint.y)},
            .size = tilemap::tileSizeOf(strolling.atlas)};

        context.image(
            antwika::ui::Icon{
                .sheetTexture = strolling.atlas == tilemap::Atlas::Wall
                              ? atlasSheets.texture(tilemap::Atlas::Wall)
                              : atlasSheets.texture(tilemap::Atlas::Floor),
                .sourceRect = walkCutRect,
                .scale = kUiScale * 2},
            kWhiteColor);
    }

}
