#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "antwika/game/BuildingKind.hpp"

namespace antwika::game
{

    /**
     * @brief What a left click lays down.
     *
     * **Simulation state, for the same reason the camera is.** What a
     * recorded click *means* depends on which tool was selected when it
     * arrived, so a replay has to arrive at the same selection rather
     * than being told it. It therefore lives in UiOverlay, is changed
     * only inside the tick path by UiSink, and no event of its own is
     * ever persisted -- see UiOverlay and Toolbar.
     *
     * This is the *palette*, and BuildingKind is the building model.
     * A road is not a building and a building kind is not something a
     * road can be laid with, so the two are separate enumerations with
     * buildingKindOf() as the one crossing between them. Folding them
     * into one gave every per-building table a Road entry that could
     * only ever be wrong.
     *
     * Road is first and is the default, so a session that never touches
     * the palette behaves exactly as one did before there was one.
     *
     * Values are contiguous from zero, so a tool can index a table.
     */
    enum class BuildTool : std::uint8_t
    {
        Road = 0,
        House,
        FoodSource,
        WaterSource,
        FireStation,
        ArchitectPost,
    };

    /**
     * @brief How many tools there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kBuildToolCount =
        static_cast<std::size_t>(BuildTool::ArchitectPost) + 1;

    /**
     * @brief Get a tool's index, for addressing a per-tool table.
     * @param tool The tool to index.
     * @return The index, always below kBuildToolCount for a named tool.
     */
    [[nodiscard]] constexpr std::size_t buildToolIndex(
        BuildTool tool) noexcept
    {
        return static_cast<std::size_t>(tool);
    }

    /**
     * @brief Get which building a tool puts up, if it puts one up.
     *
     * The buildings are the tools after Road, in the order they are
     * declared, which is the order BuildingKind declares them in.
     * So this is arithmetic rather than a switch, the way turnRight()
     * is, and adding a kind to both enumerations is the whole change.
     *
     * @param tool The tool to ask about.
     * @return The kind it places, or nullopt for Road.
     */
    [[nodiscard]] constexpr std::optional<BuildingKind> buildingKindOf(
        BuildTool tool) noexcept
    {
        if (tool == BuildTool::Road)
        {
            return std::nullopt;
        }

        return static_cast<BuildingKind>(buildToolIndex(tool) - 1);
    }

    /**
     * @brief Check whether a tool puts a building on a cell.
     * @param tool The tool to ask about.
     * @return True for every tool but Road.
     */
    [[nodiscard]] constexpr bool placesBuilding(BuildTool tool) noexcept
    {
        return buildingKindOf(tool).has_value();
    }

    // The two enumerations have to stay in step.
    // This is where adding to one and not the other fails.
    static_assert(!buildingKindOf(BuildTool::Road).has_value());
    static_assert(buildingKindOf(BuildTool::House) == BuildingKind::House);
    static_assert(
        buildingKindOf(BuildTool::ArchitectPost)
        == BuildingKind::ArchitectPost);
    static_assert(kBuildToolCount == kBuildingKindCount + 1);

} // namespace antwika::game
