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
#include <antwika/ui/TextFieldSpec.hpp>
#include <antwika/ui/Theme.hpp>

#include "antwika/map_editor/Components.hpp"
#include "antwika/map_editor/PaletteMath.hpp"
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

        constexpr std::size_t kPaletteSplit = 3;

        constexpr std::string_view kFreeName = "free";

        constexpr std::array<std::string_view, 4> kMenuTitles{
            "File", "Edit", "View", "Map"};

        constexpr std::array<std::string_view, 5> kFileEntries{
            "New",
            "Open...",
            "Save  S",
            "Save As...",
            "Quit  Esc"};

        constexpr std::array<std::string_view, 5> kFileEntriesTiles{
            "New",
            "Open...",
            "Save Sheet  S",
            "Save As...",
            "Quit  Esc"};

        constexpr std::array<std::string_view, 5> kFileEntriesChars{
            "New",
            "Open...",
            "Save Character  S",
            "Save As...",
            "Quit  Esc"};

        constexpr std::array<std::string_view, 3> kEditEntries{
            "Undo  U", "Redo  R", "Delete Entity  X"};

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

        constexpr std::array<std::string_view, 5> kMapEntries{
            "Playtest  F5",
            "Validate Now",
            "Generate  G",
            "Palette...",
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

        void describePalette(Context &ui, const EditorStore &store)
        {
            const auto describeRange =
                [&ui, &store](
                    const std::size_t first, const std::size_t last)
            {
                const auto row = ui.row({.width = kGrow});

                for (std::size_t index = first; index < last; ++index)
                {
                    const bool isFree =
                        index == widgets::kFreeBrushIndex;
                    ButtonSpec spec{
                        .id = widgets::terrainButton(index),
                        .width = kGrow};

                    const bool selected =
                        isFree
                            ? store.state.brushFree
                            : !store.state.brushFree
                                  && store.state.brush
                                         == static_cast<TerrainClass>(
                                             index);

                    if (selected)
                    {
                        spec.state = ButtonState::Pressed;
                    }

                    ui.button(
                        isFree ? kFreeName
                               : tilemap::toString(
                                   static_cast<TerrainClass>(index)),
                        spec);
                }
            };

            describeRange(0, kPaletteSplit);
            describeRange(kPaletteSplit, widgets::kPaletteCount);
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
            const auto &cell = store.state.map.at(store.state.hovered);

            ui.label(
                "cell " + std::to_string(store.state.hovered.column)
                + "," + std::to_string(store.state.hovered.row)
                + " h=" + std::to_string(cell.height));
            ui.label(
                "free: " + std::to_string(freeCells(store))
                    + " cells",
                ui.theme().muted);

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("H+", {.id = widgets::kHeightUp});
                ui.button("H-", {.id = widgets::kHeightDown});
                ui.button("Brdg", {.id = widgets::kBridge});
                ui.button("Light", {.id = widgets::kLight});
            }

            {
                const auto row = ui.row({.width = kGrow});

                ui.button("Generate", {.id = widgets::kGenerate});
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

        void describeCharacters(Context &ui, const EditorStore &store)
        {
            const auto &characters = store.characters;

            ui.label("characters");

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

        void describeEntities(Context &ui, const EditorStore &store)
        {
            ui.label("entity");

            ui.dropdown(DropdownSpec{
                .id = widgets::kKindPicker,
                .optionIdBase = widgets::kKindFirst,
                .width = kGrow,
                .options = kMarkerKindNames,
                .selected = store.ui.placeKind % kMarkerKindCount,
                .placeholder = "kind",
                .open = store.ui.placeOpen});

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

                    ui.label(
                        dialog.mode == DialogMode::Open
                            ? "Open map"
                            : "Save map as");
                    ui.label(
                        pathTail(dialog.directory),
                        ui.theme().muted);
                    describeDialogEntries(ui, store);
                    ui.textField(TextFieldSpec{
                        .id = widgets::kDialogName,
                        .width = kGrow,
                        .text = dialog.nameField.text,
                        .placeholder = "file name",
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

        constexpr std::uint32_t kSwatchWidth = 14;

        constexpr std::uint32_t kSwatchHeight = 10;

        constexpr std::uint32_t kSvWidth = 128;

        constexpr std::uint32_t kSvHeight = 64;

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
                const auto swatch = ui.panel(
                    {.width = fixedSize(kSwatchWidth),
                     .height = fixedSize(kSwatchHeight),
                     .background = colorOf(header.ink),
                     .padding = 0,
                     .gap = 0});
            }

            ui.button("Paper", paperSpec);

            {
                const auto swatch = ui.panel(
                    {.width = fixedSize(kSwatchWidth),
                     .height = fixedSize(kSwatchHeight),
                     .background = colorOf(header.paper),
                     .padding = 0,
                     .gap = 0});
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

                        ui.label("Hue", ui.theme().muted);
                        ui.slider(
                            {.id = widgets::kPaletteHue,
                             .width = kGrow,
                             .value = palette.hsv.hue,
                             .range = 359,
                             .dragging = palette.hueDragging});
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

            const std::array<std::string, 3> scaleLabels{
                scaleLabel(2, store.uiScale),
                scaleLabel(3, store.uiScale),
                scaleLabel(4, store.uiScale)};

            const std::string fullscreenLabel =
                store.fullscreen ? "Fullscreen  F10 *"
                                 : "Fullscreen  F10";

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

                    if (store.view == EditorView::Characters)
                    {
                        describeCharacters(ui, store);
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
