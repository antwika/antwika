#pragma once

#include <cstddef>
#include <cstdint>

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
     * Road is first and is the default, so a session that never touches
     * the palette behaves exactly as one did before there was one.
     *
     * Values are contiguous from zero, so a tool can index a table.
     */
    enum class BuildTool : std::uint8_t
    {
        Road = 0,
        House,
        Shop,
        Tower,
    };

    /**
     * @brief How many tools there are.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kBuildToolCount =
        static_cast<std::size_t>(BuildTool::Tower) + 1;

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
     * @brief Check whether a tool puts a building on a cell.
     *
     * Everything that is not the road is, which is why this asks about
     * the road rather than listing the buildings: a fifth tool is then a
     * building without this having to be edited.
     *
     * @param tool The tool to ask about.
     * @return True for every tool but Road.
     */
    [[nodiscard]] constexpr bool placesBuilding(BuildTool tool) noexcept
    {
        return tool != BuildTool::Road;
    }

    /**
     * @brief Get which building a tool places, counting from zero.
     *
     * The buildings are the tools after Road, in the order they are
     * declared, which is the order the atlas draws them in.
     *
     * @param tool The tool to index; Road has no building and returns
     * zero, which is the first building's index -- ask placesBuilding()
     * first.
     * @return The building's index, below kBuildToolCount - 1.
     */
    [[nodiscard]] constexpr std::size_t buildingIndex(
        BuildTool tool) noexcept
    {
        return placesBuilding(tool) ? buildToolIndex(tool) - 1 : 0;
    }

} // namespace antwika::game
