#include "antwika/map_editor/EditorState.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <variant>

#include <antwika/log/Level.hpp>
#include <antwika/tilemap/Entities.hpp>
#include <antwika/tilemap/MapFile.hpp>
#include <antwika/tilemap/MapHeader.hpp>
#include <antwika/tilemap/TileMapError.hpp>

namespace antwika::map_editor
{

    namespace
    {
        constexpr std::uint32_t kFreshColumns = 20;
        constexpr std::uint32_t kFreshRows = 11;
        constexpr std::int32_t kUnitSize = 16;

        using antwika::geometry::GridCell;
        using antwika::tilemap::TerrainClass;
        using antwika::tilemap::TileMap;

        [[nodiscard]] bool onBorder(
            const std::uint32_t column, const std::uint32_t row)
        {
            return row == 0 || row == kFreshRows - 1 || column == 0
                   || column == kFreshColumns - 1;
        }

        [[nodiscard]] TileMap freshMap()
        {
            TileMap map(
                tilemap::MapHeader{
                    .id = "untitled",
                    .ink = tilemap::Rgb{
                        .red = 214, .green = 224, .blue = 216},
                    .paper = tilemap::Rgb{
                        .red = 12, .green = 14, .blue = 16}},
                kFreshColumns,
                kFreshRows);

            for (std::uint32_t row = 0; row < kFreshRows; ++row)
            {
                for (std::uint32_t column = 0; column < kFreshColumns;
                     ++column)
                {
                    if (onBorder(column, row))
                    {
                        map.at(GridCell{.column = column, .row = row})
                            .terrain = TerrainClass::Wall;
                    }
                }
            }

            return map;
        }

        struct LoadedMap final
        {
            TileMap map;
            std::vector<bool> free{};
            bool fromFile = false;
        };

        [[nodiscard]] LoadedMap loadOrFresh(
            const std::filesystem::path &path, log::ILogger &logger)
        {
            if (!std::filesystem::exists(path))
            {
                return {.map = freshMap()};
            }

            try
            {
                auto loaded = tilemap::loadMapDocumentFile(path);

                return {
                    .map = std::move(loaded.map),
                    .free = std::move(loaded.free),
                    .fromFile = true};
            }
            catch (const tilemap::TileMapError &error)
            {
                logger.log(log::Level::Error, error.what());
                return {.map = freshMap()};
            }
        }

        template <typename KindT>
        [[nodiscard]] std::uint32_t countOf(const TileMap &map)
        {
            std::uint32_t count = 0;

            for (const auto &entity : map.entities())
            {
                if (std::holds_alternative<KindT>(entity))
                {
                    ++count;
                }
            }

            return count;
        }

        [[nodiscard]] std::uint32_t clampAxis(
            const std::int32_t pixels, const std::uint32_t cells)
        {
            if (pixels < 0)
            {
                return 0;
            }

            const auto unit =
                static_cast<std::uint32_t>(pixels / kUnitSize);

            return unit >= cells ? cells - 1 : unit;
        }
    }

    EditorState makeEditorState(
        std::filesystem::path path, log::ILogger &logger)
    {
        auto loaded = loadOrFresh(path, logger);
        auto &map = loaded.map;
        const auto transitions = countOf<tilemap::Transition>(map);
        const auto npcs = countOf<tilemap::Npc>(map);
        const auto pickups = countOf<tilemap::Pickup>(map);
        const auto boats = countOf<tilemap::BoatEmbark>(map);
        const auto spawns = countOf<tilemap::SpawnPoint>(map);
        const auto triggers = countOf<tilemap::TriggerVolume>(map);

        auto state = EditorState{
            .map = std::move(map),
            .path = std::move(path),
            .nextTransition = transitions + 1,
            .nextNpc = npcs + 1,
            .nextPickup = pickups + 1,
            .nextBoat = boats + 1,
            .nextSpawn = spawns + 1,
            .nextTrigger = triggers + 1};

        if (loaded.fromFile)
        {
            applyFreeMask(state, loaded.free);
        }
        else
        {
            pinBorder(state);
        }

        return state;
    }

    void applyFreeMask(
        EditorState &state, const std::vector<bool> &free)
    {
        const auto cells =
            static_cast<std::size_t>(state.map.columns())
            * state.map.rows();

        state.pinned.assign(cells, true);

        for (std::size_t index = 0;
             index < cells && index < free.size();
             ++index)
        {
            if (free[index])
            {
                state.pinned[index] = false;
            }
        }
    }

    std::vector<bool> freeMaskOf(const EditorState &state)
    {
        const auto cells =
            static_cast<std::size_t>(state.map.columns())
            * state.map.rows();

        std::vector<bool> free(cells, false);

        if (state.pinned.size() != cells)
        {
            return free;
        }

        for (std::size_t index = 0; index < cells; ++index)
        {
            if (!state.pinned[index])
            {
                free[index] = true;
            }
        }

        return free;
    }

    void pinAll(EditorState &state)
    {
        state.pinned.assign(
            static_cast<std::size_t>(state.map.columns())
                * state.map.rows(),
            true);
    }

    void pinBorder(EditorState &state)
    {
        const auto columns = state.map.columns();
        const auto rows = state.map.rows();

        state.pinned.assign(
            static_cast<std::size_t>(columns) * rows, false);

        for (std::uint32_t row = 0; row < rows; ++row)
        {
            for (std::uint32_t column = 0; column < columns; ++column)
            {
                if (row == 0 || row == rows - 1 || column == 0
                    || column == columns - 1)
                {
                    state.pinned
                        [static_cast<std::size_t>(row) * columns
                         + column] = true;
                }
            }
        }
    }

    void reconcilePins(EditorState &state)
    {
        const auto cells =
            static_cast<std::size_t>(state.map.columns())
            * state.map.rows();

        if (state.pinned.size() != cells)
        {
            pinAll(state);
        }
    }

    std::size_t pinIndex(
        const tilemap::TileMap &map, const geometry::GridCell cell)
    {
        return static_cast<std::size_t>(cell.row) * map.columns()
               + cell.column;
    }

    void newMap(EditorState &state)
    {
        state.map = freshMap();
        pinBorder(state);
        state.brushFree = false;
        state.undoStack.clear();
        state.redoStack.clear();
        state.hovered = GridCell{};
        state.painting = false;
        state.nextTransition = 1;
        state.nextNpc = 1;
        state.nextPickup = 1;
        state.nextBoat = 1;
        state.nextSpawn = 1;
        state.nextTrigger = 1;
        state.stampStart.reset();
        state.report.reset();
        state.reportStale = true;
    }

    geometry::GridCell cellUnder(
        const tilemap::TileMap &map, const geometry::Point canvas)
    {
        return GridCell{
            .column = clampAxis(canvas.x, map.columns()),
            .row = clampAxis(canvas.y, map.rows())};
    }

}
