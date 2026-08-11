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
        using antwika::tilemap::TileMap;

        [[nodiscard]] TileMap freshMap()
        {
            tilemap::MapHeader header;
            header.id = "untitled";
            header.ink =
                tilemap::Rgb{.red = 214, .green = 224, .blue = 216};
            header.paper =
                tilemap::Rgb{.red = 12, .green = 14, .blue = 16};

            return TileMap(
                std::move(header), kFreshColumns, kFreshRows);
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
            catch (const tilemap::TileMapError &error) // GCOVR_EXCL_LINE
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

        EditorState state{.map = std::move(map)}; // GCOVR_EXCL_LINE
        state.path = std::move(path);
        state.nextTransition = transitions + 1;
        state.nextNpc = npcs + 1;
        state.nextPickup = pickups + 1;
        state.nextBoat = boats + 1;
        state.nextSpawn = spawns + 1;
        state.nextTrigger = triggers + 1;

        if (loaded.fromFile)
        {
            applyFreeMask(state, loaded.free);
        }
        else
        {
            pinAll(state);
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
        pinAll(state);
        state.brushFree = false;
        state.activeLevel = 0;
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
        state.stamp.reset();
        state.hoveredBeyond.reset();
        state.report.reset();
        state.reportStale = true;
        ++state.revision;
    }

    geometry::GridCell cellUnder(
        const tilemap::TileMap &map, const geometry::Point canvas)
    {
        return GridCell{
            .column = clampAxis(canvas.x, map.columns()),
            .row = clampAxis(canvas.y, map.rows())};
    }

}
