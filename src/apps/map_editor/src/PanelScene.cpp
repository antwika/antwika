#include "antwika/map_editor/PanelScene.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/io/FileList.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/ui/Alignment.hpp>
#include <antwika/ui/ButtonSpec.hpp>
#include <antwika/ui/ButtonState.hpp>
#include <antwika/ui/ContainerSpec.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/DropdownSpec.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/SliderSpec.hpp>
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/Hotkeys.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
#include "antwika/map_editor/TilesetWorkspace.hpp"
#include "antwika/map_editor/Widgets.hpp"

namespace antwika::map_editor
{

    namespace
    {
        using antwika::tilemap::TerrainClass;
        using antwika::ui::ButtonSpec;
        using antwika::ui::ButtonState;
        using antwika::ui::Context;
        using antwika::ui::DropdownSpec;
        using antwika::ui::fixedSize;
        using antwika::ui::kGrow;
        using antwika::ui::TextFieldSpec;
        using antwika::ui::Theme;

        constexpr std::size_t kMaxFindings = 6;

        constexpr std::size_t kFindingWidth = 25;

        constexpr std::uint32_t kIconButtonSide = 10;

        constexpr std::uint32_t kSwatchWidth = 14;

        constexpr std::uint32_t kSwatchHeight = 10;

        constexpr std::uint32_t kSocketChipSide = 8;

        constexpr std::string_view kFreeName = "free";

        [[nodiscard]] ui::ContainerSpec swatchSpec(
            const tilemap::Rgb color)
        {
            ui::ContainerSpec spec{};

            spec.width = fixedSize(kSwatchWidth);
            spec.height = fixedSize(kSwatchHeight);
            spec.background = colorOf(color);
            spec.padding = 0;
            spec.gap = 0;

            return spec;
        }

        constexpr std::array<std::string_view, 4> kMenuTitles{
            "File", "Edit", "View", "Map"};

        constexpr std::array<std::string_view, 5> kFileEntries{
            "New",
            "Open...",
            "Save  S",
            "Save As...",
            "Quit  Esc"};

        constexpr std::array<std::string_view, 5> kFileEntriesTiles{
            "New Tileset...",
            "Open...",
            "Save Tileset  S",
            "Save As...",
            "Quit  Esc"};

        constexpr std::array<std::string_view, 5> kFileEntriesChars{
            "New",
            "Open...",
            "Save Character  S",
            "Save As...",
            "Quit  Esc"};

        constexpr std::array<std::string_view, 4> kEditEntries{
            "Undo  U", "Redo  R", "Delete Entity  X", "Keys..."};

        constexpr std::array<std::string_view, 2> kViewEntries{
            "Validator On/Off  V", "Tiles  Tab"};

        [[nodiscard]] std::string_view cycleLabelFor(
            const EditorView view)
        {
            switch (view)
            {
                case EditorView::Map:
                    return "Tiles  Tab";
                case EditorView::Tiles:
                    return "Characters  Tab";
                default:
                    return "Map  Tab";
            }
        }

        [[nodiscard]] std::string scaleLabel(
            const std::uint32_t scale, const std::uint32_t active)
        {
            return "UI Scale " + std::to_string(scale)
                   + (scale == active ? "x *" : "x");
        }

        constexpr std::array<std::string_view, 6> kMapEntries{
            "Playtest  F5",
            "Validate Now",
            "Generate  G",
            "Palette...",
            "Tilesets...",
            "Rules..."};

        constexpr std::array<
            std::span<const std::string_view>,
            widgets::kMenuCount>
            kMenuEntries{
                kFileEntries,
                kEditEntries,
                kViewEntries,
                kMapEntries};

        constexpr std::array<ui::WidgetId, widgets::kMenuCount>
            kMenuEntryBases{
                widgets::kMenuFileFirst,
                widgets::kMenuEditFirst,
                widgets::kMenuViewFirst,
                widgets::kMenuMapFirst};

        [[nodiscard]] Theme panelTheme()
        {
            Theme theme{};

            theme.face = ui::TextFace::Small;
            theme.padding = 2;
            theme.gap = 1;
            theme.buttonPadding = 2;
            theme.scrollbarWidth = 4;
            theme.sliderHeight = 8;

            return theme;
        }

        [[nodiscard]] std::string shortened(const std::string &text)
        {
            if (text.size() <= kFindingWidth)
            {
                return text;
            }

            return text.substr(0, kFindingWidth);
        }

        void iconButton(
            Context &ui, const ui::WidgetId id, const bool selected)
        {
            ButtonSpec spec{
                .id = id, .width = fixedSize(kIconButtonSide)};

            if (selected)
            {
                spec.state = ButtonState::Pressed;
            }

            ui.button(" ", spec);
        }

        void describePalette(Context &ui, const EditorStore &store)
        {
            const auto row = ui.row({.width = kGrow});

            for (std::size_t index = 0;
                 index < widgets::kPaletteCount;
                 ++index)
            {
                const bool isFree =
                    index == widgets::kFreeBrushIndex;
                const bool selected =
                    isFree
                        ? store.state.brushFree
                        : !store.state.brushFree
                              && store.state.brush
                                     == static_cast<TerrainClass>(
                                         index);

                iconButton(
                    ui, widgets::terrainButton(index), selected);
            }
        }

        [[nodiscard]] std::size_t freeCells(const EditorStore &store)
        {
            std::size_t count = 0;

            for (const auto pinnedCell : store.state.pinned)
            {
                if (!pinnedCell)
                {
                    ++count;
                }
            }

            return count;
        }

        void describeCellTools(Context &ui, const EditorStore &store)
        {
            const auto &column =
                store.state.map.at(store.state.hovered);
            const auto *top = column.top();
            const auto *slab =
                column.slabAt(store.state.activeLevel);

            std::string topText = "-";

            if (top != nullptr)
            {
                topText = std::to_string(top->level);
            }

            std::string cellText = "cell ";
            cellText += std::to_string(store.state.hovered.column);
            cellText += ",";
            cellText += std::to_string(store.state.hovered.row);
            cellText += "  top=";
            cellText += topText;

            ui.label(cellText);

            std::string slabText = "no slab";

            if (slab != nullptr)
            {
                slabText = tilemap::toString(slab->terrain);
            }

            std::string levelText = "level ";
            levelText += std::to_string(store.state.activeLevel);
            levelText += ": ";
            levelText += slabText;

            ui.label(levelText);
            ui.label(
                "free: " + std::to_string(freeCells(store))
                    + " cells",
                ui.theme().muted);

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("L+", {.id = widgets::kLevelUp});
                ui.button("L-", {.id = widgets::kLevelDown});
                ui.button("Brdg", {.id = widgets::kBridge});
                ui.button("Light", {.id = widgets::kLight});
            }

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("Generate", {.id = widgets::kGenerate});
                iconButton(
                    ui,
                    widgets::kPickerToggle,
                    store.picker.active);
                iconButton(
                    ui,
                    widgets::kMapSelectTool,
                    store.mapTool == MapTool::Select);
            }
        }

        void describeSelection(Context &ui, const EditorStore &store)
        {
            const auto &entities = store.state.map.entities();

            if (!store.ui.selected.has_value()
                || *store.ui.selected >= entities.size())
            {
                ui.label("none selected", ui.theme().muted);
                return;
            }

            const auto &entity = entities[*store.ui.selected];
            const auto kind = markerKindOf(entity);

            ui.label(
                "#" + std::to_string(*store.ui.selected) + " "
                + std::string(kMarkerKindNames
                                  [static_cast<std::size_t>(kind)]));

            ui.textField(TextFieldSpec{
                .id = widgets::kFieldId,
                .width = kGrow,
                .text = store.ui.idField.text,
                .placeholder = "id",
                .cursor = store.ui.idField.cursor});

            if (std::holds_alternative<tilemap::Transition>(entity))
            {
                ui.textField(TextFieldSpec{
                    .id = widgets::kFieldTargetMap,
                    .width = kGrow,
                    .text = store.ui.targetMapField.text,
                    .placeholder = "target map",
                    .cursor = store.ui.targetMapField.cursor});

                ui.textField(TextFieldSpec{
                    .id = widgets::kFieldTargetEntry,
                    .width = kGrow,
                    .text = store.ui.targetEntryField.text,
                    .placeholder = "target entry",
                    .cursor = store.ui.targetEntryField.cursor});
            }

            if (std::holds_alternative<tilemap::Pickup>(entity))
            {
                ui.textField(TextFieldSpec{
                    .id = widgets::kFieldTags,
                    .width = kGrow,
                    .text = store.ui.tagsField.text,
                    .placeholder = "tags a,b",
                    .cursor = store.ui.tagsField.cursor});
            }

            if (const auto *spawn =
                    std::get_if<tilemap::SpawnPoint>(&entity))
            {
                std::vector<std::string_view> names;
                names.reserve(store.characters.list.size() + 1);
                names.emplace_back("(none)");

                std::size_t selected = 0;

                for (const auto &character : store.characters.list)
                {
                    if (character.name == spawn->enemy)
                    {
                        selected = names.size();
                    }

                    names.emplace_back(character.name);
                }

                ui.dropdown(DropdownSpec{
                    .id = widgets::kEnemyPicker,
                    .optionIdBase = widgets::kEnemyFirst,
                    .width = kGrow,
                    .options = names,
                    .selected = selected,
                    .placeholder = "enemy",
                    .open = store.ui.enemyOpen});
            }
        }

        void describeDrawColor(Context &ui, const EditorStore &store)
        {
            const auto &header = store.state.map.header();
            const auto key = store.hotkeys[enums::index(
                HotkeyAction::DrawColor)];

            ui.label(
                "draw color  " + std::string(keyCaption(key)),
                ui.theme().muted);

            const auto row = ui.row(
                {.width = kGrow,
                 .cross = ui::Alignment::Center,
                 .gap = 2});

            ButtonSpec inkSpec{.id = widgets::kDrawInk};
            ButtonSpec paperSpec{.id = widgets::kDrawPaper};

            if (store.tilesets.drawPaper)
            {
                paperSpec.state = ButtonState::Pressed;
            }
            else
            {
                inkSpec.state = ButtonState::Pressed;
            }

            ui.button("ink", inkSpec);

            {
                const auto swatch = ui.panel(swatchSpec(header.ink));
            }

            ui.button("paper", paperSpec);

            {
                const auto swatch =
                    ui.panel(swatchSpec(header.paper));
            }
        }

        void describeCharacters(Context &ui, const EditorStore &store)
        {
            const auto &characters = store.characters;

            ui.label("characters");

            {
                const auto row = ui.row({.width = kGrow});

                iconButton(
                    ui,
                    widgets::kCharToolDraw,
                    characters.tool == CharacterTool::Draw);
                iconButton(
                    ui,
                    widgets::kCharToolSelect,
                    characters.tool == CharacterTool::Select);
            }

            ui.textField(TextFieldSpec{
                .id = widgets::kCharName,
                .width = kGrow,
                .text = characters.nameField.text,
                .placeholder = "name",
                .cursor = characters.nameField.cursor});

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("New", {.id = widgets::kCharNew});
                ui.button(
                    characters.confirmDelete ? "Confirm?" : "Delete",
                    {.id = widgets::kCharDelete});
            }

            if (!characters.message.empty())
            {
                ui.label(characters.message, ui.theme().focusRing);
            }

            for (std::size_t index = 0;
                 index < characters.list.size();
                 ++index)
            {
                ButtonSpec spec{
                    .id = widgets::characterRow(index),
                    .width = kGrow};

                if (index == characters.selected)
                {
                    spec.state = ButtonState::Pressed;
                }

                ui.button(characters.list[index].name, spec);
            }

            if (characters.list.empty())
            {
                ui.label("no characters", ui.theme().muted);
            }
        }

        [[nodiscard]] std::string tilesetOptionLabel(
            const TilesetDoc &doc)
        {
            return doc.data.name + (doc.dirty ? "*" : "") + " ("
                   + std::string(
                       tilemap::toString(doc.data.terrain))
                   + ")";
        }

        void describeSocketPanel(
            Context &ui,
            const EditorStore &store,
            const TilesetDoc &doc)
        {
            const auto &tilesets = store.tilesets;
            const auto &names = doc.data.socketNames;

            ui.label("sockets", ui.theme().muted);

            const auto count =
                std::min(names.size(), widgets::kSocketRowCount);

            for (std::size_t at = 0; at < count; ++at)
            {
                const auto row = ui.row(
                    {.width = kGrow,
                     .cross = ui::Alignment::Center,
                     .gap = 2});

                ui::ContainerSpec chipSpec{};

                chipSpec.width = fixedSize(kSocketChipSide);
                chipSpec.height = fixedSize(kSocketChipSide);
                chipSpec.background =
                    socketColor(static_cast<tileset::SocketId>(at));
                chipSpec.padding = 0;
                chipSpec.gap = 0;

                {
                    const auto chip = ui.panel(chipSpec);
                }

                ButtonSpec spec{
                    .id = widgets::socketRow(at), .width = kGrow};

                if (tilesets.activeSocket == at)
                {
                    spec.state = ButtonState::Pressed;
                }

                ui.button(names[at], spec);
            }

            ui.textField(TextFieldSpec{
                .id = widgets::kSocketName,
                .width = kGrow,
                .text = tilesets.socketNameField.text,
                .placeholder = "socket name",
                .cursor = tilesets.socketNameField.cursor});

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("+", {.id = widgets::kSocketAdd});
                ui.button("Ren", {.id = widgets::kSocketRename});
                ui.button("Del", {.id = widgets::kSocketDelete});
            }
        }

        void describeDecorPanel(
            Context &ui, const TilesetDoc &doc)
        {
            ui.label(
                "sits on - pick base sprites", ui.theme().muted);

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("All", {.id = widgets::kDecorAll});
                ui.button("None", {.id = widgets::kDecorNone});
            }

            const auto layer =
                std::min(doc.sel.layer, doc.data.layers.size() - 1);

            const auto row = ui.row(
                {.width = kGrow,
                 .cross = ui::Alignment::Center,
                 .gap = 2});

            ui.label("density", ui.theme().muted);
            ui.button("-", {.id = widgets::kDensityDown});
            ui.button(
                std::to_string(doc.data.layers[layer].density),
                {.id = widgets::kDensityValue});
            ui.button("+", {.id = widgets::kDensityUp});
        }

        void describeTilesetPanel(
            Context &ui, const EditorStore &store)
        {
            const auto &tilesets = store.tilesets;
            const auto *doc = activeTilesetDoc(store);

            if (doc == nullptr)
            {
                ui.label("no tilesets", ui.theme().muted);
                ui.label(
                    "File > New Tileset...", ui.theme().muted);

                if (!tilesets.message.empty())
                {
                    ui.label(
                        tilesets.message, ui.theme().focusRing);
                }

                return;
            }

            std::vector<std::string> labels{};

            for (const auto &open : tilesets.open)
            {
                if (labels.size() == widgets::kTilesetOptionCount)
                {
                    break;
                }

                labels.push_back(tilesetOptionLabel(open));
            }

            const std::vector<std::string_view> options(
                labels.begin(), labels.end());

            ui.dropdown(DropdownSpec{
                .id = widgets::kTilesetPicker,
                .optionIdBase = widgets::tilesetOption(0),
                .width = kGrow,
                .options = options,
                .selected = tilesets.active,
                .placeholder = "tileset",
                .open = tilesets.pickerOpen});

            const auto layer =
                std::min(doc->sel.layer, doc->data.layers.size() - 1);

            ui.label(
                std::string(
                    tilemap::toString(doc->data.terrain))
                    + " - L" + std::to_string(layer) + " "
                    + doc->data.layers[layer].name,
                ui.theme().muted);

            {
                const auto row = ui.row({.width = kGrow});

                iconButton(
                    ui,
                    widgets::kToolDraw,
                    tilesets.tool == TilesetTool::Draw);
                iconButton(
                    ui,
                    widgets::kToolSockets,
                    tilesets.tool == TilesetTool::Sockets);
                iconButton(
                    ui,
                    widgets::kToolDecor,
                    tilesets.tool == TilesetTool::Decor);
                iconButton(
                    ui,
                    widgets::kToolSelect,
                    tilesets.tool == TilesetTool::Select);
            }

            {
                const auto row = ui.row({.width = kGrow});
                const auto &sprites =
                    doc->data.layers[layer].sprites;
                const auto frameCount = static_cast<std::size_t>(
                    doc->sel.sprite < sprites.size()
                        ? sprites[doc->sel.sprite].frameCount
                        : 1);

                for (std::size_t frame = 0;
                     frame < widgets::kFrameButtonCount;
                     ++frame)
                {
                    ButtonSpec spec{
                        .id = widgets::frameButton(frame),
                        .width = kGrow};

                    if (doc->sel.frame == frame)
                    {
                        spec.state = ButtonState::Pressed;
                    }

                    auto text = std::to_string(frame + 1);

                    if (frame >= frameCount)
                    {
                        text += ".";
                    }

                    ui.button(text, spec);
                }

                ui.button(
                    "Clr",
                    {.id = widgets::kFrameClear, .width = kGrow});
            }

            ui.label("layers", ui.theme().muted);

            const auto layerCount = std::min(
                doc->data.layers.size(), widgets::kLayerRowCount);

            for (std::size_t at = 0; at < layerCount; ++at)
            {
                ButtonSpec spec{
                    .id = widgets::layerRow(at), .width = kGrow};

                if (at == layer)
                {
                    spec.state = ButtonState::Pressed;
                }

                ui.button(
                    "L" + std::to_string(at) + " "
                        + doc->data.layers[at].name,
                    spec);
            }

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("+Lay", {.id = widgets::kLayerAdd});
                ui.button("-Lay", {.id = widgets::kLayerRemove});
            }

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("+Spr", {.id = widgets::kSpriteAdd});
                ui.button(
                    "Dup", {.id = widgets::kSpriteDuplicate});
                ui.button(
                    tilesets.confirmDeleteSprite ? "Confirm?"
                                                 : "Del",
                    {.id = widgets::kSpriteDelete});
            }

            {
                const auto &sprites =
                    doc->data.layers[layer].sprites;

                if (doc->sel.sprite < sprites.size())
                {
                    const auto row = ui.row(
                        {.width = kGrow,
                         .cross = ui::Alignment::Center,
                         .gap = 2});

                    ui.label("weight", ui.theme().muted);
                    ui.button("-", {.id = widgets::kWeightDown});
                    ui.button(
                        std::to_string(
                            sprites[doc->sel.sprite].weight),
                        {.id = widgets::kWeightValue});
                    ui.button("+", {.id = widgets::kWeightUp});
                }
            }

            if (tilesets.tool == TilesetTool::Sockets)
            {
                describeSocketPanel(ui, store, *doc);
            }
            else if (
                tilesets.tool == TilesetTool::Decor && layer >= 1)
            {
                describeDecorPanel(ui, *doc);
            }

            if (!tilesets.message.empty())
            {
                ui.label(tilesets.message, ui.theme().focusRing);
            }
        }

        void describeEntities(Context &ui, const EditorStore &store)
        {
            ui.label("entity");

            DropdownSpec kindSpec{};

            kindSpec.id = widgets::kKindPicker;
            kindSpec.optionIdBase = widgets::kKindFirst;
            kindSpec.width = kGrow;
            kindSpec.options = kMarkerKindNames;
            kindSpec.selected =
                store.ui.placeKind % kMarkerKindCount;
            kindSpec.placeholder = "kind";
            kindSpec.open = store.ui.placeOpen;

            ui.dropdown(kindSpec);

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("Place", {.id = widgets::kPlace});
                ui.button("Delete", {.id = widgets::kDelete});
            }

            describeSelection(ui, store);
        }

        constexpr std::uint32_t kDialogWidth = 280;

        constexpr std::size_t kPathWidth = 44;

        [[nodiscard]] std::string pathTail(const std::string &path)
        {
            if (path.size() <= kPathWidth)
            {
                return path;
            }

            return "..." + path.substr(path.size() - kPathWidth + 3);
        }

        void describeDialogEntries(
            Context &ui, const EditorStore &store)
        {
            const auto &dialog = store.dialog;
            const auto first = dialog.page * kDialogRows;

            for (std::size_t row = 0; row < kDialogRows; ++row)
            {
                const auto at = first + row;

                if (at >= dialog.entries.size())
                {
                    break;
                }

                ui.button(
                    io::entryText(dialog.entries[at]),
                    {.id = widgets::dialogRow(row), .width = kGrow});
            }

            if (dialog.entries.size() <= kDialogRows)
            {
                return;
            }

            const auto pages =
                (dialog.entries.size() + kDialogRows - 1)
                / kDialogRows;
            const auto row = ui.row({.width = kGrow});

            ui.button("Prev", {.id = widgets::kDialogPrev});
            ui.label(
                std::to_string(dialog.page + 1) + "/"
                    + std::to_string(pages),
                ui.theme().muted);
            ui.spacer(kGrow);
            ui.button("Next", {.id = widgets::kDialogNext});
        }

        void describeDialog(Context &ui, const EditorStore &store)
        {
            const auto &dialog = store.dialog;

            ui.spacer(kGrow);

            {
                const auto center = ui.row({.width = kGrow});

                ui.spacer(kGrow);

                {
                    const auto box = ui.column(
                        {.width = fixedSize(kDialogWidth),
                         .background = ui.theme().panel,
                         .padding = 3,
                         .gap = 1});

                    const bool tileset =
                        dialog.target == DialogTarget::Tileset;

                    ui.label(
                        dialog.mode == DialogMode::Open
                            ? (tileset ? "Open tileset"
                                       : "Open map")
                            : (tileset ? "Save tileset as"
                                       : "Save map as"));
                    ui.label(
                        pathTail(dialog.directory),
                        ui.theme().muted);
                    describeDialogEntries(ui, store);
                    ui.textField(TextFieldSpec{
                        .id = widgets::kDialogName,
                        .width = kGrow,
                        .text = dialog.nameField.text,
                        .placeholder = tileset ? "tileset name"
                                               : "file name",
                        .cursor = dialog.nameField.cursor});

                    if (!dialog.message.empty())
                    {
                        ui.label(
                            dialog.message, ui.theme().focusRing);
                    }

                    {
                        const auto buttons = ui.row({.width = kGrow});

                        ui.button(
                            dialog.mode == DialogMode::Open
                                ? "Open"
                                : "Save",
                            {.id = widgets::kDialogConfirm});
                        ui.button(
                            "Cancel", {.id = widgets::kDialogCancel});
                    }
                }

                ui.spacer(kGrow);
            }

            ui.spacer(kGrow);
        }

        constexpr std::uint32_t kPaletteDialogWidth = 170;

        constexpr std::uint32_t kSvWidth = 128;

        constexpr std::uint32_t kSvHeight = 64;

        constexpr std::uint32_t kHueRange = 359;

        [[nodiscard]] std::string channelReadout(
            const tilemap::Rgb color)
        {
            return "R " + std::to_string(color.red) + "  G "
                   + std::to_string(color.green) + "  B "
                   + std::to_string(color.blue);
        }

        void describeSwatches(Context &ui, const EditorStore &store)
        {
            const auto &header = store.state.map.header();
            const auto row = ui.row(
                {.width = kGrow,
                 .cross = ui::Alignment::Center,
                 .gap = 2});

            ButtonSpec inkSpec{.id = widgets::kPaletteSwatchInk};
            ButtonSpec paperSpec{.id = widgets::kPaletteSwatchPaper};

            if (store.palette.paperActive)
            {
                paperSpec.state = ButtonState::Pressed;
            }
            else
            {
                inkSpec.state = ButtonState::Pressed;
            }

            ui.button("Ink", inkSpec);

            {
                const auto swatch = ui.panel(swatchSpec(header.ink));
            }

            ui.button("Paper", paperSpec);

            {
                const auto swatch =
                    ui.panel(swatchSpec(header.paper));
            }
        }

        void describePaletteDialog(
            Context &ui, const EditorStore &store)
        {
            const auto &palette = store.palette;

            ui.spacer(kGrow);

            {
                const auto center = ui.row({.width = kGrow});

                ui.spacer(kGrow);

                {
                    const auto box = ui.column(
                        {.width = fixedSize(kPaletteDialogWidth),
                         .background = ui.theme().panel,
                         .padding = 3,
                         .gap = 2});

                    ui.label("Map palette");
                    describeSwatches(ui, store);

                    {
                        const auto square = ui.panel(
                            {.width = fixedSize(kSvWidth),
                             .height = fixedSize(kSvHeight),
                             .background = gfx::Color{},
                             .padding = 0,
                             .gap = 0,
                             .id = widgets::kPaletteSv});
                    }

                    {
                        const auto row = ui.row(
                            {.width = kGrow,
                             .cross = ui::Alignment::Center,
                             .gap = 2});

                        ui::SliderSpec hueSpec{};

                        hueSpec.id = widgets::kPaletteHue;
                        hueSpec.width = kGrow;
                        hueSpec.value = palette.hsv.hue;
                        hueSpec.range = kHueRange;
                        hueSpec.dragging = palette.hueDragging;

                        ui.label("Hue", ui.theme().muted);
                        ui.slider(hueSpec);
                    }

                    ui.label(
                        channelReadout(
                            palette.paperActive
                                ? store.state.map.header().paper
                                : store.state.map.header().ink),
                        ui.theme().muted);

                    ui.textField(TextFieldSpec{
                        .id = widgets::kPaletteHex,
                        .width = kGrow,
                        .text = palette.hexField.text,
                        .placeholder = "#rrggbb",
                        .cursor = palette.hexField.cursor});

                    {
                        const auto buttons =
                            ui.row({.width = kGrow, .gap = 2});

                        ui.button(
                            "Apply", {.id = widgets::kPaletteApply});
                        ui.button(
                            "Cancel",
                            {.id = widgets::kPaletteCancel});
                    }
                }

                ui.spacer(kGrow);
            }

            ui.spacer(kGrow);
        }

        constexpr std::array<std::string_view, 6> kTerrainShort{
            "flr", "wal", "wat", "clf", "pth", "str"};

        void describeRulesMatrix(Context &ui, const EditorStore &store)
        {
            {
                const auto header = ui.row({.width = kGrow, .gap = 1});

                ui.label("    ");

                for (const auto name : kTerrainShort)
                {
                    ui.label(name, ui.theme().muted);
                }
            }

            for (std::size_t row = 0;
                 row < widgets::kRulesTerrains;
                 ++row)
            {
                const auto line = ui.row({.width = kGrow, .gap = 1});

                ui.label(kTerrainShort[row]);

                for (std::size_t column = 0;
                     column < widgets::kRulesTerrains;
                     ++column)
                {
                    ButtonSpec spec{
                        .id = widgets::rulesPairButton(row, column)};

                    if (store.rules.edit.allowed[row][column])
                    {
                        spec.state = ButtonState::Pressed;
                    }

                    ui.button(
                        store.rules.edit.allowed[row][column]
                            ? "+"
                            : "-",
                        spec);
                }
            }
        }

        void describeRulesWeights(
            Context &ui, const EditorStore &store)
        {
            for (std::size_t terrain = 0;
                 terrain < widgets::kRulesTerrains;
                 ++terrain)
            {
                if (static_cast<tilemap::TerrainClass>(terrain)
                    == tilemap::TerrainClass::Stair)
                {
                    continue;
                }

                const auto line = ui.row({.width = kGrow, .gap = 2});

                ui.label(kTerrainShort[terrain]);
                ui.button(
                    "-",
                    {.id = static_cast<ui::WidgetId>(
                         widgets::kRulesWeightDownBase + terrain)});
                ui.label(std::to_string(static_cast<std::int32_t>(
                    store.rules.edit.weights[terrain])));
                ui.button(
                    "+",
                    {.id = static_cast<ui::WidgetId>(
                         widgets::kRulesWeightUpBase + terrain)});
            }
        }

        void describeRulesDialog(
            Context &ui, const EditorStore &store)
        {
            ui.spacer(kGrow);

            {
                const auto center = ui.row({.width = kGrow});

                ui.spacer(kGrow);

                {
                    const auto box = ui.column(
                        {.width = fixedSize(180),
                         .background = ui.theme().panel,
                         .padding = 3,
                         .gap = 1});

                    ui.label("Generation rules");
                    ui.label(
                        "tileset-level, not part of map undo",
                        ui.theme().muted);
                    describeRulesMatrix(ui, store);
                    ui.label("weights", ui.theme().muted);
                    describeRulesWeights(ui, store);

                    if (!store.rules.message.empty())
                    {
                        ui.label(
                            store.rules.message,
                            ui.theme().focusRing);
                    }

                    {
                        const auto buttons =
                            ui.row({.width = kGrow, .gap = 2});

                        ui.button(
                            "Apply", {.id = widgets::kRulesApply});
                        ui.button(
                            "Cancel", {.id = widgets::kRulesCancel});
                    }
                }

                ui.spacer(kGrow);
            }

            ui.spacer(kGrow);
        }

        void describeNewTilesetDialog(
            Context &ui, const EditorStore &store)
        {
            const auto &dialog = store.newTileset;
            std::array<
                std::string_view,
                enums::kCount<TerrainClass>>
                terrains{};

            for (const auto terrain : enums::kAll<TerrainClass>)
            {
                terrains[enums::index(terrain)] =
                    tilemap::toString(terrain);
            }

            ui.spacer(kGrow);

            {
                const auto center = ui.row({.width = kGrow});

                ui.spacer(kGrow);

                {
                    const auto box = ui.column(
                        {.width = fixedSize(170),
                         .background = ui.theme().panel,
                         .padding = 3,
                         .gap = 2});

                    ui.label("New tileset");
                    ui.textField(TextFieldSpec{
                        .id = widgets::kNewTilesetName,
                        .width = kGrow,
                        .text = dialog.nameField.text,
                        .placeholder = "name",
                        .cursor = dialog.nameField.cursor});
                    ui.dropdown(DropdownSpec{
                        .id = widgets::kNewTilesetTerrain,
                        .optionIdBase = static_cast<ui::WidgetId>(
                            widgets::kNewTilesetTerrainBase),
                        .width = kGrow,
                        .options = terrains,
                        .selected = dialog.terrain
                                    % terrains.size(),
                        .placeholder = "terrain",
                        .open = dialog.terrainOpen});

                    if (!dialog.message.empty())
                    {
                        ui.label(
                            dialog.message, ui.theme().focusRing);
                    }

                    {
                        const auto buttons =
                            ui.row({.width = kGrow, .gap = 2});

                        ui.button(
                            "Create",
                            {.id = widgets::kNewTilesetCreate});
                        ui.button(
                            "Cancel",
                            {.id = widgets::kNewTilesetCancel});
                    }
                }

                ui.spacer(kGrow);
            }

            ui.spacer(kGrow);
        }

        void describeBindingsDialog(
            Context &ui, const EditorStore &store)
        {
            const auto &dialog = store.bindings;

            ui.spacer(kGrow);

            {
                const auto center = ui.row({.width = kGrow});

                ui.spacer(kGrow);

                {
                    const auto box = ui.column(
                        {.width = fixedSize(210),
                         .background = ui.theme().panel,
                         .padding = 3,
                         .gap = 1});

                    ui.label("Map tilesets");

                    std::array<
                        std::vector<std::string>,
                        enums::kCount<TerrainClass>>
                        labels{};

                    for (const auto terrain :
                         enums::kAll<TerrainClass>)
                    {
                        const auto at = enums::index(terrain);
                        auto &names = labels[at];

                        names.emplace_back("(default)");

                        for (const auto &doc :
                             store.tilesets.open)
                        {
                            if (doc.data.terrain != terrain
                                || names.size()
                                       >= widgets::
                                           kBindingOptionStride)
                            {
                                continue;
                            }

                            names.push_back(doc.data.name);
                        }
                    }

                    std::array<
                        std::vector<std::string_view>,
                        enums::kCount<TerrainClass>>
                        options{};

                    for (const auto terrain :
                         enums::kAll<TerrainClass>)
                    {
                        const auto at = enums::index(terrain);

                        options[at].assign(
                            labels[at].begin(), labels[at].end());

                        const auto line =
                            ui.row({.width = kGrow, .gap = 2});

                        ui.label(kTerrainShort[at]);
                        ui.dropdown(DropdownSpec{
                            .id = widgets::bindingPicker(at),
                            .optionIdBase =
                                widgets::bindingOption(at),
                            .width = kGrow,
                            .options = options[at],
                            .selected = dialog.chosen[at]
                                        % labels[at].size(),
                            .placeholder = "tileset",
                            .open = dialog.pickerOpen[at]});
                    }

                    if (!dialog.message.empty())
                    {
                        ui.label(
                            dialog.message, ui.theme().focusRing);
                    }

                    {
                        const auto buttons =
                            ui.row({.width = kGrow, .gap = 2});

                        ui.button(
                            "Apply",
                            {.id = widgets::kBindingsApply});
                        ui.button(
                            "Cancel",
                            {.id = widgets::kBindingsCancel});
                    }
                }

                ui.spacer(kGrow);
            }

            ui.spacer(kGrow);
        }

        void describeKeysDialog(Context &ui, const EditorStore &store)
        {
            const auto &dialog = store.keys;

            ui.spacer(kGrow);

            {
                const auto center = ui.row({.width = kGrow});

                ui.spacer(kGrow);

                {
                    const auto box = ui.column(
                        {.width = fixedSize(150),
                         .background = ui.theme().panel,
                         .padding = 3,
                         .gap = 1});

                    ui.label("Keys");
                    ui.label(
                        "click a row, press a key",
                        ui.theme().muted);

                    {
                        const auto rows = ui.column(
                            {.width = kGrow,
                             .padding = 0,
                             .gap = 0});

                        for (const auto action :
                             enums::kAll<HotkeyAction>)
                        {
                            const auto at = enums::index(action);
                            const bool capturing =
                                dialog.capturing == action;
                            ButtonSpec spec{
                                .id = widgets::keysRow(at),
                                .width = kGrow};

                            if (capturing)
                            {
                                spec.state = ButtonState::Pressed;
                            }

                            std::string caption = "press a key";

                            if (!capturing)
                            {
                                caption =
                                    keyCaption(store.hotkeys[at]);
                            }

                            std::string entry{hotkeyLabel(action)};

                            entry += "  ";
                            entry += caption;

                            ui.button(entry, spec);
                        }
                    }

                    if (!dialog.message.empty())
                    {
                        ui.label(
                            dialog.message, ui.theme().focusRing);
                    }

                    {
                        const auto buttons =
                            ui.row({.width = kGrow, .gap = 2});

                        ui.button(
                            "Defaults",
                            {.id = widgets::kKeysDefaults});
                        ui.button(
                            "Close", {.id = widgets::kKeysClose});
                    }
                }

                ui.spacer(kGrow);
            }

            ui.spacer(kGrow);
        }

        void describeMenuBar(Context &ui, const EditorStore &store)
        {
            const auto bar = ui.row(
                {.width = kGrow,
                 .height = fixedSize(
                     static_cast<std::uint32_t>(kMenuBarHeight)),
                 .background = ui.theme().panel,
                 .padding = 0,
                 .gap = 1});

            const auto activeView = store.view;

            std::array<std::string, 3> scaleLabels{};

            scaleLabels[0] = scaleLabel(2, store.uiScale);
            scaleLabels[1] = scaleLabel(3, store.uiScale);
            scaleLabels[2] = scaleLabel(4, store.uiScale);

            std::string fullscreenLabel = "Fullscreen  F10";

            if (store.fullscreen)
            {
                fullscreenLabel += " *";
            }

            const std::array<std::string_view, 6> viewOptions{
                kViewEntries[0],
                cycleLabelFor(activeView),
                scaleLabels[0],
                scaleLabels[1],
                scaleLabels[2],
                fullscreenLabel};

            for (std::size_t index = 0;
                 index < widgets::kMenuCount;
                 ++index)
            {
                auto options = kMenuEntries[index];

                if (index == 0
                    && activeView == EditorView::Tiles)
                {
                    options = kFileEntriesTiles;
                }
                else if (
                    index == 0
                    && activeView == EditorView::Characters)
                {
                    options = kFileEntriesChars;
                }
                else if (index == 2)
                {
                    options = viewOptions;
                }

                ui.dropdown(DropdownSpec{
                    .id = widgets::menuTitle(index),
                    .optionIdBase = kMenuEntryBases[index],
                    .width = ui::kFit,
                    .options = options,
                    .placeholder = kMenuTitles[index],
                    .open = store.ui.openMenu == index});
            }
        }

        void describeValidator(Context &ui, const EditorStore &store)
        {
            ui.label(
                store.state.overlayOn ? "validator on"
                                      : "validator off",
                ui.theme().muted);

            if (store.state.generateFailedTicks > 0)
            {
                ui.label("generate failed", ui.theme().focusRing);
            }

            if (!store.state.overlayOn
                || !store.state.report.has_value())
            {
                return;
            }

            std::size_t shown = 0;

            for (const auto &finding : store.state.report->findings)
            {
                if (shown == kMaxFindings)
                {
                    break;
                }

                ui.label(
                    shortened(finding.message), ui.theme().focusRing);
                ++shown;
            }

            if (shown == 0)
            {
                ui.label("no findings", ui.theme().muted);
            }
        }
    }

    ui::Frame describePanel(
        const EditorStore &store,
        const gfx::Size canvas,
        ui::Pointer pointer,
        const ui::Keyboard &keyboard)
    {
        Context ui{
            canvas,
            panelTheme(),
            pointer,
            keyboard,
            store.ui.focus};

        {
            const auto screen = ui.column(
                {.width = kGrow,
                 .height = kGrow,
                 .padding = 0,
                 .gap = 0});

            describeMenuBar(ui, store);

            {
                const auto content = ui.row(
                    {.width = kGrow,
                     .height = kGrow,
                     .padding = 0,
                     .gap = 0});

                if (store.dialog.open())
                {
                    const auto mapArea = ui.column(
                        {.width = fixedSize(
                             static_cast<std::uint32_t>(
                                 kMapViewWidth)),
                         .height = kGrow,
                         .padding = 0,
                         .gap = 0});

                    describeDialog(ui, store);
                }
                else if (store.palette.open)
                {
                    const auto mapArea = ui.column(
                        {.width = fixedSize(
                             static_cast<std::uint32_t>(
                                 kMapViewWidth)),
                         .height = kGrow,
                         .padding = 0,
                         .gap = 0});

                    describePaletteDialog(ui, store);
                }
                else if (store.rules.open)
                {
                    const auto mapArea = ui.column(
                        {.width = fixedSize(
                             static_cast<std::uint32_t>(
                                 kMapViewWidth)),
                         .height = kGrow,
                         .padding = 0,
                         .gap = 0});

                    describeRulesDialog(ui, store);
                }
                else if (store.newTileset.open)
                {
                    const auto mapArea = ui.column(
                        {.width = fixedSize(
                             static_cast<std::uint32_t>(
                                 kMapViewWidth)),
                         .height = kGrow,
                         .padding = 0,
                         .gap = 0});

                    describeNewTilesetDialog(ui, store);
                }
                else if (store.bindings.open)
                {
                    const auto mapArea = ui.column(
                        {.width = fixedSize(
                             static_cast<std::uint32_t>(
                                 kMapViewWidth)),
                         .height = kGrow,
                         .padding = 0,
                         .gap = 0});

                    describeBindingsDialog(ui, store);
                }
                else if (store.keys.open)
                {
                    const auto mapArea = ui.column(
                        {.width = fixedSize(
                             static_cast<std::uint32_t>(
                                 kMapViewWidth)),
                         .height = kGrow,
                         .padding = 0,
                         .gap = 0});

                    describeKeysDialog(ui, store);
                }
                else
                {
                    ui.spacer(fixedSize(
                        static_cast<std::uint32_t>(kMapViewWidth)));
                }

                {
                    const auto panel = ui.column(
                        {.width = kGrow,
                         .height = kGrow,
                         .background = ui.theme().panel,
                         .padding = 2,
                         .gap = 1});

                    if (store.view != EditorView::Map)
                    {
                        describeDrawColor(ui, store);
                    }

                    if (store.view == EditorView::Characters)
                    {
                        describeCharacters(ui, store);
                    }
                    else if (store.view == EditorView::Tiles)
                    {
                        describeTilesetPanel(ui, store);
                    }
                    else
                    {
                        ui.label(
                            "brush "
                            + std::string(
                                store.state.brushFree
                                    ? kFreeName
                                    : tilemap::toString(
                                        store.state.brush)));
                        describePalette(ui, store);
                        describeCellTools(ui, store);
                        describeEntities(ui, store);
                        describeValidator(ui, store);
                    }
                }
            }
        }

        return ui.finish();
    }

}
