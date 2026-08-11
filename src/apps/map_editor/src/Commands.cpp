#include "antwika/map_editor/Commands.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/mapcheck/Validate.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/ExpandedMap.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/Overlay.hpp>
#include <antwika/tilemap/Slab.hpp>
#include <antwika/tilemap/TileMapError.hpp>

#include "antwika/map_editor/PaletteMath.hpp"

namespace antwika::map_editor
{

    namespace
    {
        constexpr std::size_t kUndoDepth = 256;

        constexpr std::string_view kPlaytestBinary =
            "./build/bin/antwika_game/antwika_game";
        constexpr std::uint32_t kReportPeriod = 30;
        constexpr std::uint8_t kLightFull = 255;
        constexpr std::uint8_t kLightDim = 160;
        constexpr std::uint8_t kLightDark = 64;
        constexpr std::int32_t kLevelFloor = -32;
        constexpr std::int32_t kLevelCeiling = 32;

        using antwika::geometry::GridCell;
        using antwika::tilemap::Entity;
        using antwika::tilemap::Overlay;
        using antwika::tilemap::Slab;
        using antwika::tilemap::TileMap;

        [[nodiscard]] GridCell entityCell(const Entity &entity)
        {
            return std::visit(
                [](const auto &kind) { return kind.at; }, entity);
        }

        void pushCapped(
            std::vector<MapSnapshot> &stack, const EditorState &state)
        {
            if (stack.size() >= kUndoDepth)
            {
                stack.erase(stack.begin());
            }

            MapSnapshot taken{.map = state.map}; // GCOVR_EXCL_LINE
            taken.pinned = state.pinned;
            stack.push_back(std::move(taken));
        }

        void pushUndo(EditorState &state)
        {
            pushCapped(state.undoStack, state);
            state.redoStack.clear();
            state.reportStale = true;
            ++state.revision;
        }

        void clampHovered(EditorState &state)
        {
            state.hovered.column = std::min(
                state.hovered.column, state.map.columns() - 1);
            state.hovered.row =
                std::min(state.hovered.row, state.map.rows() - 1);
        }

        [[nodiscard]] TileMap cellsOnlyCopy(const TileMap &map)
        {
            TileMap rebuilt(map.header(), map.columns(), map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto cell =
                        GridCell{.column = column, .row = row};

                    rebuilt.at(cell) = map.at(cell);
                }
            }

            return rebuilt;
        } // GCOVR_EXCL_LINE

        [[nodiscard]] TileMap withPalette(
            const TileMap &map,
            const tilemap::Rgb ink,
            const tilemap::Rgb paper)
        {
            auto header = map.header();

            header.ink = ink;
            header.paper = paper;

            TileMap rebuilt(
                std::move(header), map.columns(), map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto cell =
                        GridCell{.column = column, .row = row};

                    rebuilt.at(cell) = map.at(cell);
                }
            }

            for (const auto &entity : map.entities())
            {
                rebuilt.addEntity(entity);
            }

            return rebuilt;
        }

        [[nodiscard]] TileMap withTilesets(
            const TileMap &map,
            const std::array<
                std::string,
                enums::kCount<tilemap::TerrainClass>> &names)
        {
            auto header = map.header();

            header.tilesets = names;

            TileMap rebuilt(
                std::move(header), map.columns(), map.rows());

            for (std::uint32_t row = 0; row < map.rows(); ++row)
            {
                for (std::uint32_t column = 0;
                     column < map.columns();
                     ++column)
                {
                    const auto cell =
                        GridCell{.column = column, .row = row};

                    rebuilt.at(cell) = map.at(cell);
                }
            }

            for (const auto &entity : map.entities())
            {
                rebuilt.addEntity(entity);
            }

            return rebuilt;
        }

        struct EntrySurface final
        {
            GridCell cell{};
            std::int32_t level = 0;
        };

        [[nodiscard]] EntrySurface entryCell(const TileMap &map)
        {
            for (const auto &entity : map.entities())
            {
                if (const auto *spawn =
                        std::get_if<tilemap::SpawnPoint>(&entity))
                {
                    return {.cell = spawn->at, .level = spawn->level};
                }
            }

            const auto fallback = GridCell{.column = 1, .row = 1};
            const auto *top = map.at(fallback).top();

            return {
                .cell = fallback,
                .level = top != nullptr ? top->level : 0};
        }
    }

    void selectBrush(
        EditorState &state, const tilemap::TerrainClass terrain)
    {
        state.brush = terrain;
        state.brushFree = false;
    }

    void selectFreeBrush(EditorState &state)
    {
        state.brushFree = true;
    }

    std::optional<ExtendResult> extendMapFor(
        EditorState &state, const SignedCell target)
    {
        const auto columns =
            static_cast<std::int32_t>(state.map.columns());
        const auto rows =
            static_cast<std::int32_t>(state.map.rows());

        if (target.column < -kExtendMargin
            || target.row < -kExtendMargin
            || target.column >= columns + kExtendMargin
            || target.row >= rows + kExtendMargin)
        {
            return std::nullopt;
        }

        const auto west =
            target.column < 0
                ? static_cast<std::uint32_t>(-target.column)
                : 0U;
        const auto north =
            target.row < 0 ? static_cast<std::uint32_t>(-target.row)
                           : 0U;
        const auto east =
            target.column >= columns
                ? static_cast<std::uint32_t>(
                      target.column - columns + 1)
                : 0U;
        const auto south =
            target.row >= rows
                ? static_cast<std::uint32_t>(target.row - rows + 1)
                : 0U;

        const auto landed = GridCell{
            .column = static_cast<std::uint32_t>(
                target.column + static_cast<std::int32_t>(west)),
            .row = static_cast<std::uint32_t>(
                target.row + static_cast<std::int32_t>(north))};

        if (west == 0 && north == 0 && east == 0 && south == 0)
        {
            return ExtendResult{.landed = landed};
        }

        reconcilePins(state);
        pushUndo(state);

        const auto oldColumns = state.map.columns();
        const auto oldRows = state.map.rows();
        const auto oldPinned = state.pinned;

        state.map = tilemap::expandedMap(
            state.map, west, north, east, south);

        const auto newColumns = state.map.columns();

        state.pinned.assign(
            static_cast<std::size_t>(newColumns) * state.map.rows(),
            false);

        for (std::uint32_t row = 0; row < oldRows; ++row)
        {
            for (std::uint32_t column = 0; column < oldColumns;
                 ++column)
            {
                const auto old =
                    static_cast<std::size_t>(row) * oldColumns
                    + column;

                state.pinned
                    [static_cast<std::size_t>(row + north)
                         * newColumns
                     + column + west] = oldPinned[old];
            }
        }

        if (state.stampStart.has_value())
        {
            state.stampStart->column += west;
            state.stampStart->row += north;
        }

        return ExtendResult{
            .landed = landed, .west = west, .north = north};
    }

    void paintExtended(EditorState &state)
    {
        reconcilePins(state);

        const auto index = pinIndex(state.map, state.hovered);

        if (state.brushFree)
        {
            state.pinned[index] = false;
            return;
        }

        state.pinned[index] = true;

        auto &column = state.map.at(state.hovered);

        if (auto *slab = column.slabAt(state.activeLevel))
        {
            slab->terrain = state.brush;
        }
        else
        {
            column.place(Slab{
                .level = state.activeLevel,
                .terrain = state.brush});
        }

        ++state.revision;
    }

    void paintHovered(EditorState &state)
    {
        reconcilePins(state);

        const auto index = pinIndex(state.map, state.hovered);

        if (state.brushFree)
        {
            state.pinned[index] = false;
            return;
        }

        state.pinned[index] = true;

        auto &column = state.map.at(state.hovered);
        auto *slab = column.slabAt(state.activeLevel);

        if (slab != nullptr && slab->terrain == state.brush)
        {
            return;
        }

        pushUndo(state);

        if (slab != nullptr)
        {
            slab->terrain = state.brush;
            return;
        }

        column.place(Slab{
            .level = state.activeLevel, .terrain = state.brush});
    }

    void applyGenerated(
        EditorState &state,
        const std::vector<tilemap::TerrainClass> &terrains)
    {
        reconcilePins(state);
        pushUndo(state);

        for (std::uint32_t row = 0; row < state.map.rows(); ++row)
        {
            for (std::uint32_t column = 0;
                 column < state.map.columns();
                 ++column)
            {
                const auto cell =
                    GridCell{.column = column, .row = row};
                const auto index = pinIndex(state.map, cell);

                if (state.pinned[index]
                    || index >= terrains.size())
                {
                    continue;
                }

                auto &stack = state.map.at(cell);

                if (auto *slab = stack.slabAt(state.activeLevel))
                {
                    slab->terrain = terrains[index];
                    continue;
                }

                stack.place(Slab{
                    .level = state.activeLevel,
                    .terrain = terrains[index]});
            }
        }
    }

    void stepActiveLevel(
        EditorState &state, const std::int32_t delta)
    {
        state.activeLevel = std::clamp(
            state.activeLevel + delta, kLevelFloor, kLevelCeiling);
    }

    void eraseSlabHovered(EditorState &state)
    {
        reconcilePins(state);

        auto &column = state.map.at(state.hovered);

        if (column.slabAt(state.activeLevel) == nullptr)
        {
            return;
        }

        state.pinned[pinIndex(state.map, state.hovered)] = true;
        pushUndo(state);
        column.remove(state.activeLevel);
    }

    void toggleBridge(EditorState &state)
    {
        auto *slab =
            state.map.at(state.hovered).slabAt(state.activeLevel);

        if (slab == nullptr)
        {
            return;
        }

        pushUndo(state);
        slab->overlay = slab->overlay == Overlay::Bridge
                            ? Overlay::None
                            : Overlay::Bridge;
    }

    void cycleLight(EditorState &state)
    {
        auto *slab =
            state.map.at(state.hovered).slabAt(state.activeLevel);

        if (slab == nullptr)
        {
            return;
        }

        pushUndo(state);

        if (slab->light == kLightFull)
        {
            slab->light = kLightDim;
        }
        else if (slab->light == kLightDim)
        {
            slab->light = kLightDark;
        }
        else
        {
            slab->light = kLightFull;
        }
    }

    void placeTransition(EditorState &state)
    {
        pushUndo(state);
        tilemap::Transition made;
        made.id = "door-" + std::to_string(state.nextTransition++);
        made.at = state.hovered;
        made.level = state.activeLevel;
        state.map.addEntity(std::move(made));
    }

    void placeNpc(EditorState &state)
    {
        pushUndo(state);
        tilemap::Npc made;
        made.id = "npc-" + std::to_string(state.nextNpc++);
        made.at = state.hovered;
        made.level = state.activeLevel;
        state.map.addEntity(std::move(made));
    }

    void placePickup(EditorState &state)
    {
        pushUndo(state);
        tilemap::Pickup made;
        made.id = "pickup-" + std::to_string(state.nextPickup++);
        made.at = state.hovered;
        made.level = state.activeLevel;
        made.item = "key";
        made.grantedTags.emplace_back("key");
        state.map.addEntity(std::move(made));
    }

    void placeEntityKind(EditorState &state, const MarkerKind kind)
    {
        switch (kind)
        {
            case MarkerKind::Transition:
                placeTransition(state);
                return;
            case MarkerKind::Boat:
            {
                pushUndo(state);

                tilemap::BoatEmbark boat;
                boat.id = "boat-" + std::to_string(state.nextBoat++);
                boat.at = state.hovered;
                boat.level = state.activeLevel;
                state.map.addEntity(std::move(boat));
                return;
            }
            case MarkerKind::Spawn:
            {
                pushUndo(state);

                tilemap::SpawnPoint spawn;
                spawn.id =
                    "spawn-" + std::to_string(state.nextSpawn++);
                spawn.at = state.hovered;
                spawn.level = state.activeLevel;
                state.map.addEntity(std::move(spawn));
                return;
            }
            case MarkerKind::Pickup:
                placePickup(state);
                return;
            case MarkerKind::Npc:
                placeNpc(state);
                return;
            case MarkerKind::Trigger:
            {
                pushUndo(state);

                tilemap::TriggerVolume trigger;
                trigger.id =
                    "trigger-" + std::to_string(state.nextTrigger++);
                trigger.at = state.hovered;
                trigger.level = state.activeLevel;
                state.map.addEntity(std::move(trigger));
                return;
            }
        }
    }

    void replaceEntity(
        EditorState &state,
        const std::size_t index,
        tilemap::Entity entity)
    {
        if (index >= state.map.entities().size())
        {
            return;
        }

        pushUndo(state);

        TileMap rebuilt = cellsOnlyCopy(state.map);

        for (std::size_t at = 0; at < state.map.entities().size();
             ++at)
        {
            rebuilt.addEntity(
                at == index ? std::move(entity)
                            : state.map.entities()[at]);
        }

        state.map = std::move(rebuilt);
    }

    void removeEntitiesAtHovered(EditorState &state)
    {
        const auto keeps = [&state](const Entity &entity)
        { return entityCell(entity) != state.hovered; };

        if (std::ranges::all_of(state.map.entities(), keeps))
        {
            return;
        }

        pushUndo(state);

        TileMap rebuilt = cellsOnlyCopy(state.map);

        for (const auto &entity : state.map.entities())
        {
            if (keeps(entity))
            {
                rebuilt.addEntity(entity);
            }
        }

        state.map = std::move(rebuilt);
    }

    void copyMapSpan(EditorStore &store, const CellSpan span)
    {
        Stamp stamp{
            .columns = span.columns,
            .rows = span.rows,
            .cells = {}};

        for (std::uint32_t row = 0; row < span.rows; ++row)
        {
            for (std::uint32_t column = 0;
                 column < span.columns;
                 ++column)
            {
                stamp.cells.push_back(store.state.map.at(GridCell{
                    .column = span.origin.column + column,
                    .row = span.origin.row + row}));
            }
        }

        store.mapClipboard = std::move(stamp);
    }

    void clearMapSpan(EditorState &state, const CellSpan span)
    {
        reconcilePins(state);
        pushUndo(state);

        for (std::uint32_t row = 0; row < span.rows; ++row)
        {
            for (std::uint32_t column = 0;
                 column < span.columns;
                 ++column)
            {
                const auto cell = GridCell{
                    .column = span.origin.column + column,
                    .row = span.origin.row + row};

                state.map.at(cell) = tilemap::Column{};
                state.pinned[pinIndex(state.map, cell)] = true;
            }
        }
    }

    void pasteMapClipboard(EditorStore &store)
    {
        auto &state = store.state;

        if (!store.mapClipboard.has_value())
        {
            return;
        }

        pushUndo(state);

        const auto &clip = *store.mapClipboard;

        for (std::uint32_t row = 0; row < clip.rows; ++row)
        {
            for (std::uint32_t column = 0;
                 column < clip.columns;
                 ++column)
            {
                const auto target = GridCell{
                    .column = state.hovered.column + column,
                    .row = state.hovered.row + row};

                if (target.column >= state.map.columns()
                    || target.row >= state.map.rows())
                {
                    continue;
                }

                state.map.at(target) = clip.cells
                    [static_cast<std::size_t>(row) * clip.columns
                     + column];
            }
        }
    }

    void moveMapSpan(
        EditorState &state,
        const CellSpan span,
        const std::int32_t deltaColumn,
        const std::int32_t deltaRow)
    {
        reconcilePins(state);
        pushUndo(state);

        std::vector<tilemap::Column> held{};

        for (std::uint32_t row = 0; row < span.rows; ++row)
        {
            for (std::uint32_t column = 0;
                 column < span.columns;
                 ++column)
            {
                const auto cell = GridCell{
                    .column = span.origin.column + column,
                    .row = span.origin.row + row};

                held.push_back(state.map.at(cell));
                state.map.at(cell) = tilemap::Column{};
                state.pinned[pinIndex(state.map, cell)] = true;
            }
        }

        const auto columns =
            static_cast<std::int32_t>(state.map.columns());
        const auto rows =
            static_cast<std::int32_t>(state.map.rows());

        for (std::uint32_t row = 0; row < span.rows; ++row)
        {
            for (std::uint32_t column = 0;
                 column < span.columns;
                 ++column)
            {
                const auto targetColumn =
                    static_cast<std::int32_t>(
                        span.origin.column + column)
                    + deltaColumn;
                const auto targetRow = static_cast<std::int32_t>(
                                           span.origin.row + row)
                                       + deltaRow;

                if (targetColumn < 0 || targetRow < 0
                    || targetColumn >= columns
                    || targetRow >= rows)
                {
                    continue;
                }

                state.map.at(GridCell{
                    .column =
                        static_cast<std::uint32_t>(targetColumn),
                    .row = static_cast<std::uint32_t>(
                        targetRow)}) = held
                    [static_cast<std::size_t>(row) * span.columns
                     + column];
            }
        }
    }

    void markStampStart(EditorState &state)
    {
        state.stampStart = state.hovered;
    }

    void copyStampEnd(EditorState &state)
    {
        if (!state.stampStart.has_value())
        {
            return;
        }

        const auto left =
            std::min(state.stampStart->column, state.hovered.column);
        const auto right =
            std::max(state.stampStart->column, state.hovered.column);
        const auto top =
            std::min(state.stampStart->row, state.hovered.row);
        const auto bottom =
            std::max(state.stampStart->row, state.hovered.row);

        Stamp stamp{
            .columns = right - left + 1,
            .rows = bottom - top + 1,
            .cells = {}};

        for (auto row = top; row <= bottom; ++row)
        {
            for (auto column = left; column <= right; ++column)
            {
                stamp.cells.push_back(state.map.at(
                    GridCell{.column = column, .row = row}));
            }
        }

        state.stamp = std::move(stamp);
    }

    void pasteStamp(EditorState &state)
    {
        if (!state.stamp.has_value())
        {
            return;
        }

        pushUndo(state);

        for (std::uint32_t row = 0; row < state.stamp->rows; ++row)
        {
            for (std::uint32_t column = 0;
                 column < state.stamp->columns;
                 ++column)
            {
                const auto target = GridCell{
                    .column = state.hovered.column + column,
                    .row = state.hovered.row + row};

                if (target.column >= state.map.columns()
                    || target.row >= state.map.rows())
                {
                    continue;
                }

                state.map.at(target) = state.stamp->cells
                    [static_cast<std::size_t>(row)
                         * state.stamp->columns
                     + column];
            }
        }
    }

    void setPalette(
        EditorState &state,
        const tilemap::Rgb ink,
        const tilemap::Rgb paper)
    {
        if (state.map.header().ink == ink
            && state.map.header().paper == paper)
        {
            return;
        }

        pushUndo(state);
        state.map = withPalette(state.map, ink, paper);
    }

    void setTilesets(
        EditorState &state,
        const std::array<
            std::string,
            enums::kCount<tilemap::TerrainClass>> &names)
    {
        if (state.map.header().tilesets == names)
        {
            return;
        }

        pushUndo(state);
        state.map = withTilesets(state.map, names);
    }

    void previewPalette(
        EditorState &state,
        const tilemap::Rgb ink,
        const tilemap::Rgb paper)
    {
        if (state.map.header().ink == ink
            && state.map.header().paper == paper)
        {
            return;
        }

        state.map = withPalette(state.map, ink, paper);
        ++state.revision;
    }

    tilemap::Rgb activePaletteColor(const EditorStore &store)
    {
        return store.palette.paperActive
                   ? store.state.map.header().paper
                   : store.state.map.header().ink;
    }

    void syncPaletteFromActive(EditorStore &store)
    {
        const auto color = activePaletteColor(store);

        store.palette.hsv = hsvOfRgb(color);
        store.palette.hexField.text = hexOfRgb(color);
        store.palette.hexField.cursor =
            store.palette.hexField.text.size();
    }

    void pickPaletteColor(
        EditorStore &store,
        const tilemap::Rgb color,
        const bool refreshHex)
    {
        const auto &header = store.state.map.header();

        previewPalette(
            store.state,
            store.palette.paperActive ? header.ink : color,
            store.palette.paperActive ? color : header.paper);

        if (refreshHex)
        {
            store.palette.hexField.text = hexOfRgb(color);
            store.palette.hexField.cursor =
                store.palette.hexField.text.size();
        }
    }

    void openPaletteDialog(EditorStore &store)
    {
        PaletteDialog fresh;
        fresh.open = true;
        fresh.savedInk = store.state.map.header().ink;
        fresh.savedPaper = store.state.map.header().paper;
        store.palette = std::move(fresh);
        syncPaletteFromActive(store);
    }

    void applyPaletteDialog(EditorStore &store)
    {
        const auto pickedInk = store.state.map.header().ink;
        const auto pickedPaper = store.state.map.header().paper;

        previewPalette(
            store.state,
            store.palette.savedInk,
            store.palette.savedPaper);
        setPalette(store.state, pickedInk, pickedPaper);
        store.palette.open = false;
    }

    void cancelPaletteDialog(EditorStore &store)
    {
        previewPalette(
            store.state,
            store.palette.savedInk,
            store.palette.savedPaper);
        store.palette.open = false;
    }

    void undo(EditorState &state)
    {
        if (state.undoStack.empty())
        {
            return;
        }

        pushCapped(state.redoStack, state);
        state.map = std::move(state.undoStack.back().map);
        state.pinned = std::move(state.undoStack.back().pinned);
        state.undoStack.pop_back();
        clampHovered(state);
        reconcilePins(state);
        state.reportStale = true;
        ++state.revision;
    }

    void redo(EditorState &state)
    {
        if (state.redoStack.empty())
        {
            return;
        }

        pushCapped(state.undoStack, state);
        state.map = std::move(state.redoStack.back().map);
        state.pinned = std::move(state.redoStack.back().pinned);
        state.redoStack.pop_back();
        clampHovered(state);
        reconcilePins(state);
        state.reportStale = true;
        ++state.revision;
    }

    void toggleOverlay(EditorState &state)
    {
        state.overlayOn = !state.overlayOn;
    }

    void saveMap(EditorState &state, log::ILogger &logger)
    {
        try
        {
            tilemap::MapDocument document{.map = state.map}; // GCOVR_EXCL_LINE
            document.free = freeMaskOf(state);

            tilemap::saveMapFile(state.path, document);
            logger.log(
                log::Level::Info, "saved " + state.path.string());
        }
        catch (const tilemap::TileMapError &error) // GCOVR_EXCL_LINE
        {
            logger.log(log::Level::Error, error.what());
        }
    }

    std::optional<std::string> openMapAt(
        EditorState &state,
        const std::filesystem::path &path,
        log::ILogger &logger)
    {
        try
        {
            auto loaded = tilemap::loadMapDocumentFile(path);

            pushUndo(state);
            state.map = std::move(loaded.map);
            state.path = path;
            state.activeLevel = 0;
            clampHovered(state);
            applyFreeMask(state, loaded.free);
            logger.log(
                log::Level::Info, "opened " + path.string());
            return std::nullopt;
        }
        catch (const tilemap::TileMapError &error) // GCOVR_EXCL_LINE
        {
            logger.log(log::Level::Error, error.what());
            return std::string{error.what()};
        }
    }

    std::optional<std::string> saveMapAt(
        EditorState &state,
        const std::filesystem::path &path,
        log::ILogger &logger)
    {
        try
        {
            tilemap::MapDocument document{.map = state.map}; // GCOVR_EXCL_LINE
            document.free = freeMaskOf(state);

            tilemap::saveMapFile(path, document);
            state.path = path;
            logger.log(log::Level::Info, "saved " + path.string());
            return std::nullopt;
        }
        catch (const tilemap::TileMapError &error) // GCOVR_EXCL_LINE
        {
            logger.log(log::Level::Error, error.what());
            return std::string{error.what()};
        }
    }

    void playtest(EditorState &state, log::ILogger &logger)
    {
        saveMap(state, logger);

        const auto command = std::string(kPlaytestBinary)
                             + " --map \"" + state.path.string()
                             + "\" &";

        logger.log(log::Level::Info, "launching " + command);

        if (std::system(command.c_str()) != 0) // GCOVR_EXCL_LINE
        {
            logger.log( // GCOVR_EXCL_LINE
                log::Level::Warning, // GCOVR_EXCL_LINE
                "playtest launch failed"); // GCOVR_EXCL_LINE
        }
    }

    void reloadMap(EditorState &state, log::ILogger &logger)
    {
        try
        {
            auto loaded =
                tilemap::loadMapDocumentFile(state.path);

            pushUndo(state);
            state.map = std::move(loaded.map);
            state.activeLevel = 0;
            clampHovered(state);
            applyFreeMask(state, loaded.free);
            logger.log(
                log::Level::Info, "reloaded " + state.path.string());
        }
        catch (const tilemap::TileMapError &error) // GCOVR_EXCL_LINE
        {
            logger.log(log::Level::Error, error.what());
        }
    }

    void validateNow(EditorState &state)
    {
        const auto entry = entryCell(state.map);

        state.report = mapcheck::validateMap(
            state.map, entry.cell, entry.level, {});
        state.reportStale = false;
        state.framesSinceReport = 0;
    }

    void refreshReport(EditorState &state)
    {
        ++state.framesSinceReport;

        if (!state.overlayOn || !state.reportStale)
        {
            return;
        }

        if (state.report.has_value()
            && state.framesSinceReport < kReportPeriod)
        {
            return;
        }

        const auto entry = entryCell(state.map);

        state.report = mapcheck::validateMap(
            state.map, entry.cell, entry.level, {});
        state.reportStale = false;
        state.framesSinceReport = 0;
    }

}
