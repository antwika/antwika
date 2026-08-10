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
#include <antwika/tilemap/TileMapError.hpp>

#include "antwika/map_editor/PaletteMath.hpp"

namespace antwika::map_editor
{

    namespace
    {
        constexpr std::size_t kUndoDepth = 256;

        constexpr std::string_view kPlaytestCommand =
            "./build/bin/antwika_tilemap_demo/antwika_tilemap_demo &";
        constexpr std::uint32_t kReportPeriod = 30;
        constexpr std::uint8_t kLightFull = 255;
        constexpr std::uint8_t kLightDim = 160;
        constexpr std::uint8_t kLightDark = 64;

        using antwika::geometry::GridCell;
        using antwika::tilemap::Entity;
        using antwika::tilemap::Overlay;
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

            stack.push_back(MapSnapshot{
                .map = state.map, .pinned = state.pinned});
        }

        void pushUndo(EditorState &state)
        {
            pushCapped(state.undoStack, state);
            state.redoStack.clear();
            state.reportStale = true;
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
        }

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

        [[nodiscard]] GridCell entryCell(const TileMap &map)
        {
            for (const auto &entity : map.entities())
            {
                if (std::holds_alternative<tilemap::SpawnPoint>(entity))
                {
                    return entityCell(entity);
                }
            }

            return GridCell{.column = 1, .row = 1};
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
        state.map.at(state.hovered).terrain = state.brush;
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

        if (state.map.at(state.hovered).terrain == state.brush)
        {
            return;
        }

        pushUndo(state);
        state.map.at(state.hovered).terrain = state.brush;
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

                state.map.at(cell).terrain = terrains[index];
            }
        }
    }

    void raiseHovered(EditorState &state)
    {
        pushUndo(state);
        ++state.map.at(state.hovered).height;
    }

    void lowerHovered(EditorState &state)
    {
        pushUndo(state);
        --state.map.at(state.hovered).height;
    }

    void toggleBridge(EditorState &state)
    {
        pushUndo(state);

        auto &cell = state.map.at(state.hovered);

        cell.overlay = cell.overlay == Overlay::Bridge
                           ? Overlay::None
                           : Overlay::Bridge;
    }

    void cycleLight(EditorState &state)
    {
        pushUndo(state);

        auto &cell = state.map.at(state.hovered);

        if (cell.light == kLightFull)
        {
            cell.light = kLightDim;
        }
        else if (cell.light == kLightDim)
        {
            cell.light = kLightDark;
        }
        else
        {
            cell.light = kLightFull;
        }
    }

    void placeTransition(EditorState &state)
    {
        pushUndo(state);
        state.map.addEntity(tilemap::Transition{
            .id = "door-" + std::to_string(state.nextTransition++),
            .at = state.hovered});
    }

    void placeNpc(EditorState &state)
    {
        pushUndo(state);
        state.map.addEntity(tilemap::Npc{
            .id = "npc-" + std::to_string(state.nextNpc++),
            .at = state.hovered});
    }

    void placePickup(EditorState &state)
    {
        pushUndo(state);
        state.map.addEntity(tilemap::Pickup{
            .id = "pickup-" + std::to_string(state.nextPickup++),
            .at = state.hovered,
            .item = "key",
            .grantedTags = {"key"}});
    }

    void placeEntityKind(EditorState &state, const MarkerKind kind)
    {
        switch (kind)
        {
            case MarkerKind::Transition:
                placeTransition(state);
                return;
            case MarkerKind::Boat:
                pushUndo(state);
                state.map.addEntity(tilemap::BoatEmbark{
                    .id = "boat-" + std::to_string(state.nextBoat++),
                    .at = state.hovered});
                return;
            case MarkerKind::Spawn:
                pushUndo(state);
                state.map.addEntity(tilemap::SpawnPoint{
                    .id = "spawn-" + std::to_string(state.nextSpawn++),
                    .at = state.hovered});
                return;
            case MarkerKind::Pickup:
                placePickup(state);
                return;
            case MarkerKind::Npc:
                placeNpc(state);
                return;
            case MarkerKind::Trigger:
                pushUndo(state);
                state.map.addEntity(tilemap::TriggerVolume{
                    .id = "trigger-"
                          + std::to_string(state.nextTrigger++),
                    .at = state.hovered});
                return;
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
        store.palette = PaletteDialog{
            .open = true,
            .savedInk = store.state.map.header().ink,
            .savedPaper = store.state.map.header().paper};
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
    }

    void toggleOverlay(EditorState &state)
    {
        state.overlayOn = !state.overlayOn;
    }

    void saveMap(EditorState &state, log::ILogger &logger)
    {
        try
        {
            tilemap::saveMapFile(
                state.path,
                tilemap::MapDocument{
                    .map = state.map, .free = freeMaskOf(state)});
            logger.log(
                log::Level::Info, "saved " + state.path.string());
        }
        catch (const tilemap::TileMapError &error)
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
            clampHovered(state);
            applyFreeMask(state, loaded.free);
            logger.log(
                log::Level::Info, "opened " + path.string());
            return std::nullopt;
        }
        catch (const tilemap::TileMapError &error)
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
            tilemap::saveMapFile(
                path,
                tilemap::MapDocument{
                    .map = state.map, .free = freeMaskOf(state)});
            state.path = path;
            logger.log(log::Level::Info, "saved " + path.string());
            return std::nullopt;
        }
        catch (const tilemap::TileMapError &error)
        {
            logger.log(log::Level::Error, error.what());
            return std::string{error.what()};
        }
    }

    void playtest(EditorState &state, log::ILogger &logger)
    {
        saveMap(state, logger);

        const std::string command{kPlaytestCommand};

        logger.log(log::Level::Info, "launching " + command);

        if (std::system(command.c_str()) != 0)
        {
            logger.log(log::Level::Warning, "playtest launch failed");
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
            clampHovered(state);
            applyFreeMask(state, loaded.free);
            logger.log(
                log::Level::Info, "reloaded " + state.path.string());
        }
        catch (const tilemap::TileMapError &error)
        {
            logger.log(log::Level::Error, error.what());
        }
    }

    void validateNow(EditorState &state)
    {
        state.report = mapcheck::validateMap(
            state.map, entryCell(state.map), {});
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

        state.report = mapcheck::validateMap(
            state.map, entryCell(state.map), {});
        state.reportStale = false;
        state.framesSinceReport = 0;
    }

}
