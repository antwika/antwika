#include "antwika/map_editor/Hints.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <utility>
#include <span>
#include <string_view>
#include <variant>

#include <antwika/autotile/Connectors.hpp>
#include <antwika/enums/Enumeration.hpp>

#include <antwika/tilemap/Cell.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::geometry::GridCell;
        using antwika::tilemap::TerrainClass;
        using antwika::ui::kNoWidget;
        using antwika::ui::WidgetId;

        constexpr std::array<std::string_view, 7> kBrushHints{
            "brush: floor",
            "brush: wall",
            "brush: water",
            "brush: cliff",
            "brush: path",
            "brush: stair",
            "brush: free (unpin for generation)"};

        constexpr std::array<std::string_view, 4> kMenuTitleHints{
            "file: new, open, save, save as, quit",
            "edit: undo, redo, delete entity",
            "view: validator, views, ui scale, fullscreen",
            "map: playtest, validate, generate, palette"};

        constexpr std::array<std::string_view, 5> kFileEntryHints{
            "start a fresh map",
            "open a map file",
            "save to the current file",
            "save to a new file",
            "quit the editor"};

        constexpr std::array<std::string_view, 3> kEditEntryHints{
            "undo the last change",
            "redo the undone change",
            "delete entities on the hovered cell"};

        constexpr std::array<std::string_view, 6> kViewEntryHints{
            "toggle the validator overlay",
            "cycle map, tiles, characters",
            "window at 2x scale",
            "window at 3x scale",
            "window at 4x scale",
            "toggle fullscreen"};

        constexpr std::array<std::string_view, 5> kMapEntryHints{
            "save and launch the demo",
            "run the validator now",
            "generate: fill all free cells",
            "pick the ink and paper colors",
            "edit the tileset generation rules"};

        [[nodiscard]] std::optional<std::string_view> rangeHint(
            const WidgetId id,
            const WidgetId first,
            const std::span<const std::string_view> hints)
        {
            const auto raw = static_cast<std::uint64_t>(id);
            const auto base = static_cast<std::uint64_t>(first);

            if (raw < base || raw >= base + hints.size())
            {
                return std::nullopt;
            }

            return hints[raw - base];
        }

        [[nodiscard]] std::string dialogRowHint(
            const EditorStore &store, const std::size_t row)
        {
            const auto &dialog = store.dialog;
            const auto at = dialog.page * kDialogRows + row;

            if (at >= dialog.entries.size())
            {
                return {};
            }

            const auto &entry = dialog.entries[at];

            return entry.directory ? "enter " + entry.name
                                   : "pick " + entry.name;
        }

        [[nodiscard]] std::string widgetHint(
            const EditorStore &store, const WidgetId id)
        {
            if (const auto brush = rangeHint(
                    id, widgets::terrainButton(0), kBrushHints))
            {
                return std::string(*brush);
            }

            if (const auto title = rangeHint(
                    id,
                    widgets::menuTitle(0),
                    kMenuTitleHints))
            {
                return std::string(*title);
            }

            if (const auto entry = rangeHint(
                    id, widgets::kMenuFileFirst, kFileEntryHints))
            {
                return std::string(*entry);
            }

            if (const auto entry = rangeHint(
                    id, widgets::kMenuEditFirst, kEditEntryHints))
            {
                return std::string(*entry);
            }

            if (const auto entry = rangeHint(
                    id, widgets::kMenuViewFirst, kViewEntryHints))
            {
                return std::string(*entry);
            }

            if (const auto entry = rangeHint(
                    id, widgets::kMenuMapFirst, kMapEntryHints))
            {
                return std::string(*entry);
            }

            if (const auto row =
                    widgets::dialogRowIndex(id, kDialogRows))
            {
                return dialogRowHint(store, *row);
            }

            if (const auto row = widgets::characterRowIndex(
                    id, store.characters.list.size()))
            {
                return "select character "
                       + store.characters.list[*row].name;
            }

            if (id == widgets::kHeightUp)
            {
                return "raise the hovered cell";
            }

            if (id == widgets::kHeightDown)
            {
                return "lower the hovered cell";
            }

            if (id == widgets::kBridge)
            {
                return "toggle a bridge on the hovered cell";
            }

            if (id == widgets::kLight)
            {
                return "cycle light on the hovered cell";
            }

            if (id == widgets::kGenerate)
            {
                return "generate: fill all free cells";
            }

            if (id == widgets::kKindPicker)
            {
                return "choose the entity kind to place";
            }

            if (id == widgets::kPlace)
            {
                return "place an entity at the hovered cell";
            }

            if (id == widgets::kDelete)
            {
                return "delete entities at the hovered cell";
            }

            if (id == widgets::kFieldId)
            {
                return "the selected entity's id";
            }

            if (id == widgets::kFieldTargetMap)
            {
                return "the transition's target map";
            }

            if (id == widgets::kFieldTargetEntry)
            {
                return "the transition's target entry";
            }

            if (id == widgets::kFieldTags)
            {
                return "the pickup's granted tags";
            }

            if (id == widgets::kDialogPrev)
            {
                return "previous page";
            }

            if (id == widgets::kDialogNext)
            {
                return "next page";
            }

            if (id == widgets::kDialogConfirm)
            {
                return store.dialog.mode == DialogMode::Open
                           ? "open the named file"
                           : "save to the named file";
            }

            if (id == widgets::kDialogCancel
                || id == widgets::kPaletteCancel)
            {
                return "cancel";
            }

            if (id == widgets::kDialogName)
            {
                return "type the file name";
            }

            if (id == widgets::kPaletteSwatchInk)
            {
                return "edit the ink color";
            }

            if (id == widgets::kPaletteSwatchPaper)
            {
                return "edit the paper color";
            }

            if (id == widgets::kPaletteHue)
            {
                return "slide the hue";
            }

            if (id == widgets::kPaletteSv)
            {
                return "pick saturation and value";
            }

            if (id == widgets::kPaletteHex)
            {
                return "type a #rrggbb color";
            }

            if (id == widgets::kPaletteApply)
            {
                return "apply the palette as one undoable edit";
            }

            if (const auto pair = widgets::rulesPairIndex(id))
            {
                const auto row =
                    *pair / widgets::kRulesTerrains;
                const auto column =
                    *pair % widgets::kRulesTerrains;

                return "toggle "
                       + std::string(tilemap::toString(
                           static_cast<TerrainClass>(row)))
                       + "-"
                       + std::string(tilemap::toString(
                           static_cast<TerrainClass>(column)))
                       + " adjacency (tileset rules)";
            }

            {
                const auto raw =
                    static_cast<std::uint64_t>(id);

                if (raw >= widgets::kRulesWeightDownBase
                    && raw < widgets::kRulesWeightDownBase
                                 + widgets::kRulesTerrains)
                {
                    return "lower the "
                           + std::string(tilemap::toString(
                               static_cast<TerrainClass>(
                                   raw
                                   - widgets::
                                       kRulesWeightDownBase)))
                           + " weight";
                }

                if (raw >= widgets::kRulesWeightUpBase
                    && raw < widgets::kRulesWeightUpBase
                                 + widgets::kRulesTerrains)
                {
                    return "raise the "
                           + std::string(tilemap::toString(
                               static_cast<TerrainClass>(
                                   raw
                                   - widgets::
                                       kRulesWeightUpBase)))
                           + " weight";
                }
            }

            if (id == widgets::kRulesApply)
            {
                return "write rules.json for this tileset";
            }

            if (id == widgets::kRulesCancel)
            {
                return "discard the rule edits";
            }

            if (id == widgets::kCharName)
            {
                return "type a character name";
            }

            if (id == widgets::kCharNew)
            {
                return "create the named character";
            }

            if (id == widgets::kCharDelete)
            {
                return "delete the selected character";
            }

            if (id == widgets::kEnemyPicker)
            {
                return "choose the spawn's enemy character";
            }

            return {};
        }

        [[nodiscard]] std::optional<SignedCell> signedCellAt(
            const EditorStore &store, const gfx::Point canvas)
        {
            if (canvas.x < 0 || canvas.x >= kMapViewWidth
                || canvas.y < kMenuBarHeight)
            {
                return std::nullopt;
            }

            const auto zoom = store.camera.zoom();
            const auto mapX =
                (static_cast<float>(canvas.x) - store.camera.panX)
                / zoom;
            const auto mapY =
                (static_cast<float>(canvas.y - kMenuBarHeight)
                 - store.camera.panY)
                / zoom;

            return SignedCell{
                .column = static_cast<std::int32_t>(
                    std::floor(mapX / 16.0F)),
                .row = static_cast<std::int32_t>(
                    std::floor(mapY / 16.0F))};
        }

        [[nodiscard]] std::string cellHint(
            const EditorStore &store, const GridCell at)
        {
            const auto &map = store.state.map;
            const auto &cell = map.at(at);

            std::string hint =
                "cell " + std::to_string(at.column) + ","
                + std::to_string(at.row) + "  "
                + std::string(tilemap::toString(cell.terrain))
                + " h=" + std::to_string(cell.height);

            if (cell.overlay == tilemap::Overlay::Bridge)
            {
                hint += "  bridge";
            }

            if (cell.light != 255)
            {
                hint += "  light " + std::to_string(cell.light);
            }

            const auto index = pinIndex(map, at);

            if (index < store.state.pinned.size()
                && !store.state.pinned[index])
            {
                hint += "  free";
            }

            if (cell.water.deadly)
            {
                hint += "  deadly";
            }

            if (cell.water.swimmable)
            {
                hint += "  swimmable";
            }

            if (cell.water.current.has_value())
            {
                hint += "  current "
                        + std::string(
                            tilemap::toString(*cell.water.current));
            }

            for (const auto &entity : map.entities())
            {
                if (entityCellOf(entity) != at)
                {
                    continue;
                }

                const auto kind = markerKindOf(entity);
                const auto id = std::visit(
                    [](const auto &value) { return value.id; },
                    entity);

                hint += "  " + id + " "
                        + std::string(
                            kMarkerKindNames[static_cast<std::size_t>(
                                kind)]);
            }

            return hint;
        }

        [[nodiscard]] std::string mapHint(const EditorStore &store)
        {
            const auto &pointer = store.input.canvasPointer;

            if (!pointer.has_value())
            {
                return {};
            }

            const auto cell = signedCellAt(store, *pointer);

            if (!cell.has_value())
            {
                return {};
            }

            const auto columns =
                static_cast<std::int32_t>(store.state.map.columns());
            const auto rows =
                static_cast<std::int32_t>(store.state.map.rows());
            const bool inside = cell->column >= 0 && cell->row >= 0
                                && cell->column < columns
                                && cell->row < rows;

            if (inside)
            {
                return cellHint(
                    store,
                    GridCell{
                        .column = static_cast<std::uint32_t>(
                            cell->column),
                        .row = static_cast<std::uint32_t>(
                            cell->row)});
            }

            if (store.state.hoveredBeyond.has_value())
            {
                return "paint to extend the map";
            }

            return {};
        }

        [[nodiscard]] std::string tilesHint(const EditorStore &store)
        {
            const auto &pointer = store.input.canvasPointer;

            if (!pointer.has_value())
            {
                return {};
            }

            const auto pixel = sheetPixelAt(*pointer);

            if (!pixel.has_value())
            {
                return {};
            }

            const auto &doc =
                store.tiles.docs[enums::index(store.state.brush)];

            auto hint = slotLabelAt(*pixel) + "  px "
                        + std::to_string(pixel->x) + ","
                        + std::to_string(pixel->y);

            if (sheetPixelInked(doc.image, *pixel))
            {
                hint += "  ink";
            }

            if (const auto slot = variantSlotAt(*pixel))
            {
                const auto edges =
                    store.tiles
                        .connectors[enums::index(
                            store.state.brush)]
                        .edges[static_cast<std::size_t>(*slot)];

                hint += "  edges ";

                if (edges == 0)
                {
                    hint += "none";
                }
                else
                {
                    const std::array<
                        std::pair<std::uint8_t, char>,
                        4>
                        letters{
                            {{autotile::kEdgeNorth, 'N'},
                             {autotile::kEdgeEast, 'E'},
                             {autotile::kEdgeSouth, 'S'},
                             {autotile::kEdgeWest, 'W'}}};
                    bool first = true;

                    for (const auto &[bit, letter] : letters)
                    {
                        if ((edges & bit) == 0)
                        {
                            continue;
                        }

                        if (!first)
                        {
                            hint += ",";
                        }

                        hint += letter;
                        first = false;
                    }
                }
            }

            return hint;
        }

        [[nodiscard]] std::string charactersHint(
            const EditorStore &store)
        {
            const auto &pointer = store.input.canvasPointer;
            const auto &characters = store.characters;

            if (!pointer.has_value()
                || characters.selected >= characters.list.size())
            {
                return {};
            }

            const auto pixel = characterPixelAt(*pointer);

            if (!pixel.has_value())
            {
                return {};
            }

            const auto row =
                pixel->y / static_cast<std::int32_t>(kFrameSize);
            const auto frame =
                pixel->x / static_cast<std::int32_t>(kFrameSize);

            auto hint = std::string(rowNameOf(row)) + " frame "
                        + std::to_string(frame) + "  px "
                        + std::to_string(pixel->x) + ","
                        + std::to_string(pixel->y);

            if (sheetPixelInked(
                    characters.list[characters.selected]
                        .sheet.image,
                    *pixel))
            {
                hint += "  ink";
            }

            return hint;
        }
    }

    HintKey hintKeyFor(
        const EditorStore &store, const ui::WidgetId hovered)
    {
        std::uint8_t connectorEdges = 0;

        if (store.view == EditorView::Tiles
            && store.input.canvasPointer.has_value())
        {
            if (const auto pixel =
                    sheetPixelAt(*store.input.canvasPointer))
            {
                if (const auto slot = variantSlotAt(*pixel))
                {
                    connectorEdges =
                        store.tiles
                            .connectors[enums::index(
                                store.state.brush)]
                            .edges[static_cast<std::size_t>(
                                *slot)];
                }
            }
        }

        return HintKey{
            .view = store.view,
            .widget = hovered,
            .pointer = store.input.canvasPointer,
            .modal = modalOpen(store),
            .edits = store.state.undoStack.size()
                     + store.state.redoStack.size(),
            .connectorEdges = connectorEdges};
    }

    std::string hintFor(
        const EditorStore &store, const ui::WidgetId hovered)
    {
        if (store.input.consoleVisible
            && store.input.canvasPointer.has_value()
            && store.input.canvasPointer->y
                   < store.input.consoleHeightCanvas)
        {
            return {};
        }

        if (hovered != kNoWidget)
        {
            return widgetHint(store, hovered);
        }

        if (modalOpen(store))
        {
            return {};
        }

        switch (store.view)
        {
            case EditorView::Map:
                return mapHint(store);
            case EditorView::Tiles:
                return tilesHint(store);
            default:
                return charactersHint(store);
        }
    }

}
