#include "antwika/map_editor/Hints.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
#include <variant>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tileset/Sprite.hpp>

#include "antwika/map_editor/CharacterSheets.hpp"
#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/Selection.hpp"
#include "antwika/map_editor/SheetWorkspace.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
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
            "floor brush (1)",
            "wall brush (2)",
            "water brush (3)",
            "cliff brush (4)",
            "path brush (5)",
            "stair brush (6)",
            "free brush (7) - unpin for generation"};

        constexpr std::array<std::string_view, 4> kMenuTitleHints{
            "file: new, open, save, save as, quit",
            "edit: undo, redo, delete entity, keys",
            "view: validator, views, ui scale, fullscreen",
            "map: playtest, validate, generate, palette"};

        constexpr std::array<std::string_view, 5> kFileEntryHints{
            "start a fresh map",
            "open a map file",
            "save to the current file",
            "save to a new file",
            "quit the editor"};

        constexpr std::array<std::string_view, 5>
            kFileEntryHintsTiles{
                "create a new tileset",
                "open a tileset directory",
                "save the active tileset",
                "save the tileset under a new name",
                "quit the editor"};

        constexpr std::array<std::string_view, 4> kEditEntryHints{
            "undo the last change",
            "redo the undone change",
            "delete entities on the hovered cell",
            "rebind the editor hotkeys"};

        constexpr std::array<std::string_view, 6> kViewEntryHints{
            "toggle the validator overlay",
            "cycle map, tiles, characters",
            "window at 2x scale",
            "window at 3x scale",
            "window at 4x scale",
            "toggle fullscreen"};

        constexpr std::array<std::string_view, 6> kMapEntryHints{
            "save and launch the demo",
            "run the validator now",
            "generate: fill free cells on the active level",
            "pick the ink and paper colors",
            "bind tilesets to the map's terrains",
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

        [[nodiscard]] std::string socketNameOf(
            const tileset::Tileset &data,
            const tileset::SocketId socket)
        {
            if (socket < data.socketNames.size())
            {
                return data.socketNames[socket];
            }

            return "?";
        }

        [[nodiscard]] std::string tilesetWidgetHint(
            const EditorStore &store, const WidgetId id)
        {
            const auto *doc = activeTilesetDoc(store);

            if (id == widgets::kTilesetPicker)
            {
                return "switch the active tileset";
            }

            if (widgets::rangeIndex(
                    id,
                    widgets::kTilesetOptionBase,
                    widgets::kTilesetOptionCount))
            {
                return "activate this tileset";
            }

            if (id == widgets::kToolDraw)
            {
                return "draw tool: paint sprite pixels";
            }

            if (id == widgets::kToolSockets)
            {
                return "socket tool: tag sprite edges";
            }

            if (id == widgets::kToolDecor)
            {
                return "decor tool: pick the bases decor sits on";
            }

            if (id == widgets::kToolSelect)
            {
                return "select";
            }

            if (const auto frame = widgets::rangeIndex(
                    id,
                    widgets::kFrameButtonBase,
                    widgets::kFrameButtonCount))
            {
                return "select frame " + std::to_string(*frame + 1)
                       + " - first stroke copies frame 1";
            }

            if (id == widgets::kFrameClear)
            {
                return "clear frame 1 or delete trailing frames";
            }

            if (id == widgets::kLayerAdd)
            {
                return "add a decor layer";
            }

            if (id == widgets::kLayerRemove)
            {
                return "remove the selected decor layer";
            }

            if (const auto row = widgets::rangeIndex(
                    id,
                    widgets::kLayerRowBase,
                    widgets::kLayerRowCount))
            {
                return "select layer " + std::to_string(*row);
            }

            if (id == widgets::kSpriteAdd)
            {
                return "add a blank sprite to this layer";
            }

            if (id == widgets::kSpriteDuplicate)
            {
                return "duplicate the selected sprite";
            }

            if (id == widgets::kSpriteDelete)
            {
                return "delete the selected sprite (click twice)";
            }

            if (id == widgets::kSocketName)
            {
                return "type a socket name";
            }

            if (id == widgets::kSocketAdd)
            {
                return "add the named socket";
            }

            if (id == widgets::kSocketRename)
            {
                return "rename the selected socket";
            }

            if (id == widgets::kSocketDelete)
            {
                return "delete the selected unused socket";
            }

            if (const auto row = widgets::rangeIndex(
                    id,
                    widgets::kSocketRowBase,
                    widgets::kSocketRowCount))
            {
                std::string name{"?"};

                if (doc != nullptr)
                {
                    name = socketNameOf(
                        doc->data,
                        static_cast<tileset::SocketId>(*row));
                }

                return "paint edges with the " + name + " socket";
            }

            if (id == widgets::kDecorAll)
            {
                return "allow this decor on every base sprite";
            }

            if (id == widgets::kDecorNone)
            {
                return "allow this decor on no base sprite";
            }

            if (id == widgets::kDensityDown)
            {
                return "lower the decor density by 16";
            }

            if (id == widgets::kDensityValue)
            {
                return "decor scatter density, 0 to 255";
            }

            if (id == widgets::kDensityUp)
            {
                return "raise the decor density by 16";
            }

            if (id == widgets::kWeightDown)
            {
                return "lower the sprite's weight by 1";
            }

            if (id == widgets::kWeightValue)
            {
                return "how often this sprite is chosen relative "
                       "to its peers";
            }

            if (id == widgets::kWeightUp)
            {
                return "raise the sprite's weight by 1";
            }

            if (id == widgets::kNewTilesetName)
            {
                return "type the new tileset's name";
            }

            if (id == widgets::kNewTilesetTerrain)
            {
                return "choose the tileset's terrain";
            }

            if (widgets::rangeIndex(
                    id,
                    widgets::kNewTilesetTerrainBase,
                    enums::kCount<TerrainClass>))
            {
                return "make a tileset for this terrain";
            }

            if (id == widgets::kNewTilesetCreate)
            {
                return "create the named tileset";
            }

            if (id == widgets::kNewTilesetCancel
                || id == widgets::kBindingsCancel)
            {
                return "cancel";
            }

            if (id == widgets::kBindingsApply)
            {
                return "apply the bindings as one undoable edit";
            }

            if (widgets::rangeIndex(
                    id,
                    widgets::kBindingPickerBase,
                    enums::kCount<TerrainClass>))
            {
                return "choose this terrain's tileset";
            }

            if (widgets::rangeIndex(
                    id,
                    widgets::kBindingOptionBase,
                    enums::kCount<TerrainClass>
                        * widgets::kBindingOptionStride))
            {
                return "bind this tileset";
            }

            return {};
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
                if (store.view == EditorView::Tiles)
                {
                    return std::string(kFileEntryHintsTiles
                        [static_cast<std::uint64_t>(id)
                         - static_cast<std::uint64_t>(
                             widgets::kMenuFileFirst)]);
                }

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

            {
                auto hint = tilesetWidgetHint(store, id);

                if (!hint.empty())
                {
                    return hint;
                }
            }

            if (id == widgets::kLevelUp)
            {
                return "step the active level up";
            }

            if (id == widgets::kLevelDown)
            {
                return "step the active level down";
            }

            if (id == widgets::kBridge)
            {
                return "toggle a bridge on the active level's slab";
            }

            if (id == widgets::kLight)
            {
                return "cycle light on the active level's slab";
            }

            if (id == widgets::kGenerate)
            {
                return "generate: fill free cells on the active"
                       " level";
            }

            if (id == widgets::kPickerToggle)
            {
                const auto key = store.hotkeys[enums::index(
                    HotkeyAction::Picker)];

                return "sprite picker "
                       + std::string(keyCaption(key))
                       + ": map clicks pick the sprite under them";
            }

            if (id == widgets::kMapSelectTool
                || id == widgets::kCharToolSelect)
            {
                return "select";
            }

            if (id == widgets::kCharToolDraw)
            {
                return "draw tool: paint sheet pixels";
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

            if (const auto row = widgets::rangeIndex(
                    id, widgets::kKeysRowBase, kHotkeyActionCount))
            {
                return "rebind "
                       + std::string(hotkeyLabel(
                           enums::at<HotkeyAction>(*row)))
                       + " - click, then press the new key";
            }

            if (id == widgets::kKeysDefaults)
            {
                return "restore the default keys";
            }

            if (id == widgets::kKeysClose)
            {
                return "close the keys dialog";
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

        [[nodiscard]] std::string terrainTextOf(
            const tilemap::Slab *slab)
        {
            if (slab == nullptr)
            {
                return "no slab";
            }

            return std::string(tilemap::toString(slab->terrain));
        }

        [[nodiscard]] std::string levelTextOf(
            const tilemap::Slab *slab)
        {
            if (slab == nullptr)
            {
                return "-";
            }

            return std::to_string(slab->level);
        }

        [[nodiscard]] std::string entityTextOf(
            const tilemap::Entity &entity)
        {
            const auto kind = markerKindOf(entity);
            const auto id = std::visit(
                [](const auto &value) { return value.id; }, entity);

            return "  " + id + " "
                   + std::string(kMarkerKindNames[static_cast<
                       std::size_t>(kind)])
                   + " L" + std::to_string(entityLevelOf(entity));
        }

        [[nodiscard]] std::string cellHint(
            const EditorStore &store, const GridCell at)
        {
            const auto &map = store.state.map;
            const auto &column = map.at(at);
            const auto *slab =
                column.slabAt(store.state.activeLevel);

            std::string hint =
                "cell " + std::to_string(at.column) + ","
                + std::to_string(at.row) + "  L"
                + std::to_string(store.state.activeLevel) + " "
                + terrainTextOf(slab);

            if (slab != nullptr)
            {
                if (slab->overlay == tilemap::Overlay::Bridge)
                {
                    hint += "  bridge";
                }

                if (slab->light != 255)
                {
                    hint +=
                        "  light " + std::to_string(slab->light);
                }

                if (slab->water.deadly)
                {
                    hint += "  deadly";
                }

                if (slab->water.swimmable)
                {
                    hint += "  swimmable";
                }

                if (slab->water.current.has_value())
                {
                    hint += "  current "
                            + std::string(tilemap::toString(
                                *slab->water.current));
                }

                hint += "  right-click erases";
            }

            hint += "  top=" + levelTextOf(column.top());

            const auto index = pinIndex(map, at);

            if (index < store.state.pinned.size()
                && !store.state.pinned[index])
            {
                hint += "  free";
            }

            std::string entities;

            for (const auto &entity : map.entities())
            {
                if (entityCellOf(entity) == at)
                {
                    entities += entityTextOf(entity);
                }
            }

            return hint + entities;
        }

        [[nodiscard]] std::string sizeText(
            const std::uint32_t width, const std::uint32_t height)
        {
            return std::to_string(width) + "x"
                   + std::to_string(height);
        }

        [[nodiscard]] std::string selectionHint(
            const std::uint32_t width, const std::uint32_t height)
        {
            return "selection " + sizeText(width, height)
                   + " - drag moves, ctrl+x/c/v";
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

            if (store.picker.active)
            {
                return store.picker.hover;
            }

            if (store.mapSelection.dragging)
            {
                const auto span = cellSpanOf(
                    store.mapSelection.anchor,
                    store.mapSelection.focus);

                return "selecting "
                       + sizeText(span.columns, span.rows);
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
                const auto at = GridCell{
                    .column =
                        static_cast<std::uint32_t>(cell->column),
                    .row = static_cast<std::uint32_t>(cell->row)};
                const auto span = mapSelectionSpan(store);

                if (span.has_value()
                    && cellSpanContains(*span, at))
                {
                    return selectionHint(
                        span->columns, span->rows);
                }

                return cellHint(store, at);
            }

            if (store.state.hoveredBeyond.has_value())
            {
                return "paint to extend the map";
            }

            return {};
        }

        [[nodiscard]] std::string drawingSuffix(
            const EditorStore &store)
        {
            return store.tilesets.drawPaper ? "  drawing paper"
                                            : "  drawing ink";
        }

        [[nodiscard]] std::string pixelClassText(
            const PixelClass value)
        {
            switch (value)
            {
                case PixelClass::Ink:
                    return "ink";
                case PixelClass::Paper:
                    return "paper";
                default:
                    return "blank";
            }
        }

        [[nodiscard]] std::string libraryCellHint(
            const EditorStore &store,
            const TilesetDoc &doc,
            const std::size_t cell)
        {
            const auto &tilesets = store.tilesets;
            const bool decor =
                tilesets.tool == TilesetTool::Decor
                && doc.sel.layer >= 1;
            const auto layerAt =
                decor ? 0
                      : std::min(
                          doc.sel.layer,
                          doc.data.layers.size() - 1);
            const auto &layer = doc.data.layers[layerAt];
            const auto at =
                tilesets.libraryPage * kLibraryPageSize + cell;

            if (at > layer.sprites.size())
            {
                return {};
            }

            if (at == layer.sprites.size())
            {
                if (decor)
                {
                    return {};
                }

                return "add a sprite";
            }

            const auto &sprite = layer.sprites[at];

            if (decor)
            {
                const auto &decorLayer =
                    doc.data.layers[std::min(
                        doc.sel.layer,
                        doc.data.layers.size() - 1)];
                const auto *held =
                    doc.sel.sprite < decorLayer.sprites.size()
                        ? &decorLayer.sprites[doc.sel.sprite]
                        : nullptr;
                const bool allowed =
                    held != nullptr
                    && std::ranges::find(held->on, sprite.id)
                           != held->on.end();

                return allowed
                           ? "decor sits on this - click removes"
                           : "decor skips this - click adds";
            }

            auto hint = "sprite " + std::to_string(at) + " - L"
                        + std::to_string(layerAt) + " "
                        + layer.name;

            if (sprite.weight != tileset::kDefaultWeight)
            {
                hint += " w" + std::to_string(sprite.weight);
            }

            const std::array<std::string_view, 4> sides{
                "n", "e", "s", "w"};

            for (std::size_t side = 0; side < 4; ++side)
            {
                hint += " ";
                hint += sides[side];
                hint += " "
                        + socketNameOf(
                            doc.data, sprite.sockets[side]);
            }

            return hint;
        }

        [[nodiscard]] std::string bandHint(
            const EditorStore &store,
            const TilesetDoc &doc,
            const tileset::Side side)
        {
            const auto &layer = doc.data.layers[std::min(
                doc.sel.layer, doc.data.layers.size() - 1)];

            if (doc.sel.sprite >= layer.sprites.size())
            {
                return {};
            }

            const auto socket =
                layer.sprites[doc.sel.sprite]
                    .sockets[enums::index(side)];
            auto hint = std::string(tileset::toString(side))
                        + " edge: "
                        + socketNameOf(doc.data, socket);

            if (store.tilesets.tool != TilesetTool::Sockets)
            {
                return hint + " - use the Sock tool to change";
            }

            const auto &active = store.tilesets.activeSocket;

            if (!active.has_value())
            {
                return hint + " - pick a socket first";
            }

            if (*active == socket)
            {
                return hint + " - click clears to open";
            }

            return hint + " - click sets "
                   + socketNameOf(
                       doc.data,
                       static_cast<tileset::SocketId>(*active));
        }

        [[nodiscard]] std::string tilesHint(const EditorStore &store)
        {
            const auto &pointer = store.input.canvasPointer;
            const auto *doc = activeTilesetDoc(store);

            if (!pointer.has_value() || doc == nullptr)
            {
                return {};
            }

            if (store.tilesSelection.pixels.dragging)
            {
                const auto span = pixelSpanOf(
                    store.tilesSelection.pixels.anchor,
                    store.tilesSelection.pixels.focus);

                return "selecting "
                       + sizeText(
                           static_cast<std::uint32_t>(span.width),
                           static_cast<std::uint32_t>(
                               span.height));
            }

            if (const auto pixel = editorPixelAt(*pointer))
            {
                const auto span = tilesSelectionSpan(store);

                if (span.has_value()
                    && pixelSpanContains(*span, *pixel))
                {
                    return selectionHint(
                        static_cast<std::uint32_t>(span->width),
                        static_cast<std::uint32_t>(
                            span->height));
                }
            }

            if (const auto pixel = editorPixelAt(*pointer))
            {
                const auto &layer = doc->data.layers[std::min(
                    doc->sel.layer, doc->data.layers.size() - 1)];
                auto hint = "px " + std::to_string(pixel->x) + ","
                            + std::to_string(pixel->y);

                if (doc->sel.sprite < layer.sprites.size())
                {
                    const auto &sprite =
                        layer.sprites[doc->sel.sprite];

                    if (doc->sel.frame < sprite.frameCount)
                    {
                        hint += "  "
                                + pixelClassText(
                                    sprite.frames[doc->sel.frame]
                                        .pixels[static_cast<
                                            std::size_t>(
                                            pixel->y * 8
                                            + pixel->x)]);
                    }
                    else
                    {
                        hint += "  absent frame";
                    }
                }

                return hint + drawingSuffix(store);
            }

            if (const auto side = socketBandAt(*pointer))
            {
                return bandHint(store, *doc, *side);
            }

            if (const auto frame = framePreviewAt(*pointer))
            {
                const auto &layer = doc->data.layers[std::min(
                    doc->sel.layer, doc->data.layers.size() - 1)];
                const bool present =
                    doc->sel.sprite < layer.sprites.size()
                    && *frame < layer.sprites[doc->sel.sprite]
                                    .frameCount;

                return "frame " + std::to_string(*frame + 1)
                       + (present
                              ? " - click to edit"
                              : " absent - first stroke copies"
                                " frame 1");
            }

            if (pointer->x >= 140 && pointer->x < 164
                && pointer->y >= 190 && pointer->y < 214)
            {
                return "animation preview";
            }

            if (overPreviewRegen(*pointer))
            {
                return "regenerate the preview";
            }

            if (overPreviewAuto(*pointer))
            {
                return "cycle new combinations automatically";
            }

            if (overPreview(*pointer))
            {
                return "preview - sprite "
                       + std::to_string(doc->sel.sprite)
                       + " in a generated combination";
            }

            if (const auto cell = libraryCellAt(*pointer))
            {
                return libraryCellHint(store, *doc, *cell);
            }

            return {};
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

            if (store.charSelection.pixels.dragging)
            {
                const auto span = pixelSpanOf(
                    store.charSelection.pixels.anchor,
                    store.charSelection.pixels.focus);

                return "selecting "
                       + sizeText(
                           static_cast<std::uint32_t>(span.width),
                           static_cast<std::uint32_t>(
                               span.height));
            }

            const auto pixel = characterPixelAt(*pointer);

            if (!pixel.has_value())
            {
                return {};
            }

            {
                const auto span = charSelectionSpan(store);

                if (span.has_value()
                    && pixelSpanContains(*span, *pixel))
                {
                    return selectionHint(
                        static_cast<std::uint32_t>(span->width),
                        static_cast<std::uint32_t>(
                            span->height));
                }
            }

            const auto row =
                pixel->y / static_cast<std::int32_t>(kFrameSize);
            const auto frame =
                pixel->x / static_cast<std::int32_t>(kFrameSize);

            auto hint = std::string(rowNameOf(row)) + " frame "
                        + std::to_string(frame) + "  px "
                        + std::to_string(pixel->x) + ","
                        + std::to_string(pixel->y);

            hint += "  "
                    + pixelClassText(sheetPixelClass(
                        characters.list[characters.selected]
                            .sheet.image,
                        *pixel));

            return hint + drawingSuffix(store);
        }
    }

    HintKey hintKeyFor(
        const EditorStore &store, const ui::WidgetId hovered)
    {
        std::size_t tilesState = 0;

        if (store.view == EditorView::Tiles)
        {
            const auto mix = [&tilesState](const std::size_t value)
            {
                tilesState =
                    tilesState * 1099511628211ULL + value + 1;
            };

            const auto &tilesets = store.tilesets;

            mix(static_cast<std::size_t>(tilesets.tool));
            mix(tilesets.activeSocket.value_or(
                static_cast<std::size_t>(-1)));
            mix(tilesets.libraryPage);
            mix(tilesets.drawPaper ? 1 : 0);

            if (const auto *doc = activeTilesetDoc(store))
            {
                mix(static_cast<std::size_t>(doc->revision));
                mix(doc->sel.layer);
                mix(doc->sel.sprite);
                mix(doc->sel.frame);
            }
        }

        std::size_t selectionState = 0;
        const auto mixSelection =
            [&selectionState](const std::size_t value)
        {
            selectionState =
                selectionState * 1099511628211ULL + value + 1;
        };

        if (store.view == EditorView::Map)
        {
            const auto &sel = store.mapSelection;

            mixSelection(sel.dragging ? 1U : 0U);
            mixSelection(sel.moving ? 1U : 0U);

            if (sel.rect.has_value())
            {
                mixSelection(sel.rect->origin.column);
                mixSelection(sel.rect->origin.row);
                mixSelection(sel.rect->columns);
                mixSelection(sel.rect->rows);
            }
        }
        else
        {
            const auto &sel =
                store.view == EditorView::Tiles
                    ? store.tilesSelection.pixels
                    : store.charSelection.pixels;

            mixSelection(sel.dragging ? 1U : 0U);
            mixSelection(sel.moving ? 1U : 0U);

            if (sel.rect.has_value())
            {
                mixSelection(static_cast<std::size_t>(
                    sel.rect->origin.x));
                mixSelection(static_cast<std::size_t>(
                    sel.rect->origin.y));
                mixSelection(static_cast<std::size_t>(
                    sel.rect->width));
                mixSelection(static_cast<std::size_t>(
                    sel.rect->height));
            }
        }

        return HintKey{
            .view = store.view,
            .widget = hovered,
            .pointer = store.input.canvasPointer,
            .modal = modalOpen(store),
            .edits = store.state.undoStack.size()
                     + store.state.redoStack.size(),
            .level = store.state.activeLevel,
            .tilesState = tilesState,
            .selectionState = selectionState,
            .pickerState =
                store.picker.active
                    ? std::hash<std::string>{}(store.picker.hover)
                          | 1
                    : 0};
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
