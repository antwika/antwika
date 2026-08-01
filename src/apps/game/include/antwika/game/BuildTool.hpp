#pragma once

#include <array>
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
        Farm,
        ClayPit,
        Workshop,
        Storage,
        Market,
        Well,
        Doctor,
        FireStation,
        EngineerPost,
    };

    /**
     * @brief How many tools there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kBuildToolCount =
        static_cast<std::size_t>(BuildTool::EngineerPost) + 1;

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
     * **A table rather than the arithmetic it used to be.** While every
     * tool but the road placed a building, and in BuildingKind's own
     * order, `BuildingKind(index - 1)` was exact and a static_assert on
     * the two counts kept it that way. It is exact only by coincidence:
     * the moment the palette carries a tool that places nothing -- a
     * bulldozer, a roadblock, an overlay -- the offset is wrong for
     * every entry after it, and wrong silently, because the result is
     * still a valid enumerator. A table cannot be wrong that way.
     *
     * @param tool The tool to ask about.
     * @return The kind it places, or nullopt for a tool that places
     * none.
     */
    [[nodiscard]] constexpr std::optional<BuildingKind> buildingKindOf(
        BuildTool tool) noexcept
    {
        constexpr std::array<
            std::optional<BuildingKind>, kBuildToolCount> places{
            std::nullopt,                  // Road
            BuildingKind::House,
            BuildingKind::Farm,
            BuildingKind::ClayPit,
            BuildingKind::Workshop,
            BuildingKind::Storage,
            BuildingKind::Market,
            BuildingKind::Well,
            BuildingKind::Doctor,
            BuildingKind::FireStation,
            BuildingKind::EngineerPost};

        return places[buildToolIndex(tool) % kBuildToolCount];
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

    // Every building kind has to be placeable.
    // Otherwise it is art nobody can put down.
    // The table above is the only place that can go wrong.
    // So this is where it does.
    static_assert(
        []
        {
            for (std::size_t kind = 0; kind < kBuildingKindCount; ++kind)
            {
                bool found = false;

                for (std::size_t tool = 0; tool < kBuildToolCount; ++tool)
                {
                    found = found
                        || buildingKindOf(static_cast<BuildTool>(tool))
                               == static_cast<BuildingKind>(kind);
                }

                if (!found)
                {
                    return false;
                }
            }

            return true;
        }(),
        "every building kind needs a tool that places it");

    static_assert(!buildingKindOf(BuildTool::Road).has_value());
    static_assert(buildingKindOf(BuildTool::House) == BuildingKind::House);
    static_assert(
        buildingKindOf(BuildTool::EngineerPost)
        == BuildingKind::EngineerPost);

} // namespace antwika::game
