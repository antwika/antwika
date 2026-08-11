#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include <antwika/geometry/Grid.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/mapcheck/Validate.hpp>
#include <antwika/tilemap/Column.hpp>
#include <antwika/tilemap/TerrainClass.hpp>
#include <antwika/tilemap/TileMap.hpp>

#include "antwika/map_editor/GenerationRules.hpp"

namespace antwika::map_editor
{

    struct Stamp final
    {
        std::uint32_t columns = 0;
        std::uint32_t rows = 0;
        std::vector<tilemap::Column> cells{};
    };

    struct MapSnapshot final
    {
        tilemap::TileMap map;
        std::vector<bool> pinned{};
    };

    struct SignedCell final
    {
        std::int32_t column = 0;
        std::int32_t row = 0;
    };

    struct EditorState final
    {
        tilemap::TileMap map;
        std::filesystem::path path{};

        /**
         * @brief A change counter for render caching.
         *
         * Ensures: bumped by every command that changes what the
         *          map renders as, so equal values mean an
         *          unchanged picture.
         */
        std::uint64_t revision = 0;
        tilemap::TerrainClass brush = tilemap::TerrainClass::Floor;
        bool brushFree = false;

        /**
         * @brief The level edits act on, as session state.
         *
         * Ensures: reset to zero when a map is created, opened, or
         *          reloaded, and untouched by undo and redo.
         */
        std::int32_t activeLevel = 0;
        std::vector<bool> pinned{};
        GenerationRules rules = defaultGenerationRules();
        std::uint32_t generateSeed = 1;
        std::uint32_t generateFailedTicks = 0;
        geometry::GridCell hovered{};
        std::optional<SignedCell> hoveredBeyond{};
        bool painting = false;
        std::vector<MapSnapshot> undoStack{};
        std::vector<MapSnapshot> redoStack{};
        std::uint32_t nextTransition = 1;
        std::uint32_t nextNpc = 1;
        std::uint32_t nextPickup = 1;
        std::uint32_t nextBoat = 1;
        std::uint32_t nextSpawn = 1;
        std::uint32_t nextTrigger = 1;
        std::optional<geometry::GridCell> stampStart{};
        std::optional<Stamp> stamp{};
        bool overlayOn = false;
        std::optional<mapcheck::MapReport> report{};
        bool reportStale = true;
        std::uint32_t framesSinceReport = 0;
    };

    /**
     * @brief Builds the editor state for the given map path.
     *
     * @param path The map file the editor loads from and saves to.
     * @param logger Receives a message when loading the file fails.
     * @return The loaded map, or a fresh blank map when the file
     *         does not exist or fails to load.
     */
    [[nodiscard]] EditorState makeEditorState(
        std::filesystem::path path, log::ILogger &logger);

    /**
     * @brief Replaces the map with a fresh blank one.
     *
     * Ensures: every cell holds a single floor slab at level zero,
     *          every cell is pinned, the active level returns to
     *          zero, the undo and redo stacks are cleared, the
     *          counters restart at one, the stamp and selection
     *          state reset, and the file on disk is untouched.
     */
    void newMap(EditorState &state);

    void pinAll(EditorState &state);

    /**
     * @brief Rebuilds the pin grid from a serialized free mask.
     *
     * Ensures: cells beyond the mask's length load as pinned.
     */
    void applyFreeMask(
        EditorState &state, const std::vector<bool> &free);

    /**
     * @brief The serialized free mask matching the pin grid.
     *
     * Ensures: a pin grid that mismatches the map serializes as
     *          all-pinned.
     */
    [[nodiscard]] std::vector<bool> freeMaskOf(
        const EditorState &state);

    /**
     * @brief Repairs the pin grid after the map changed shape.
     *
     * Ensures: a pin grid that no longer matches the map's cell
     *          count is replaced by an all-pinned one.
     */
    void reconcilePins(EditorState &state);

    [[nodiscard]] std::size_t pinIndex(
        const tilemap::TileMap &map, geometry::GridCell cell);

    /**
     * @brief Finds the map cell under a canvas position.
     *
     * @param map The map whose bounds clamp the result.
     * @param canvas The pointer position in canvas pixels.
     * @return The nearest cell inside the map bounds.
     *
     * Ensures: the height lift is ignored, so a lifted cell resolves
     *          to the cell under the pointer's ground position.
     */
    [[nodiscard]] geometry::GridCell cellUnder(
        const tilemap::TileMap &map, geometry::Point canvas);

}
