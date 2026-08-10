#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <antwika/gfx/Bitmap.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/RectF.hpp>
#include <antwika/gfx/ViewportRenderer.hpp>
#include <antwika/log/ILogger.hpp>

#include "antwika/map_editor/EditorStore.hpp"

namespace antwika::map_editor
{

    inline constexpr std::uint32_t kCharacterSize = 64;

    inline constexpr std::uint32_t kFrameSize = 16;

    inline constexpr std::int32_t kCharacterZoom = 4;

    /**
     * @brief Names a direction row of the 4x4 frame table.
     */
    [[nodiscard]] std::string_view rowNameOf(std::int32_t row);

    /**
     * @brief Finds the character sheet pixel under a canvas point.
     */
    [[nodiscard]] std::optional<gfx::Point> characterPixelAt(
        gfx::Point canvas) noexcept;

    /**
     * @brief Builds the procedural silhouette placeholder sheet.
     *
     * Ensures: every frame holds a 1-bit figure whose legs alternate
     *          by frame and whose face marks the row's direction.
     */
    [[nodiscard]] gfx::Bitmap placeholderCharacter();

    /**
     * @brief Loads every 64x64 PNG in the characters directory.
     *
     * Ensures: files of the wrong size are skipped with a warning
     *          and the list is sorted by name.
     */
    [[nodiscard]] std::vector<CharacterDoc> loadCharacters(
        const std::filesystem::path &directory, log::ILogger &logger);

    /**
     * @brief Writes a character's PNG and JSON sidecar.
     *
     * @return An error message, or nothing on success.
     */
    [[nodiscard]] std::optional<std::string> saveCharacter(
        const CharacterDoc &character,
        const std::filesystem::path &directory);

    /**
     * @brief Removes a character's PNG and JSON sidecar.
     */
    void deleteCharacterFiles(
        const std::string &name,
        const std::filesystem::path &directory);

    /**
     * @brief Saves the selected character's PNG and sidecar.
     *
     * Ensures: success clears the dirty flag and updates the panel
     *          message on failure.
     */
    void saveSelectedCharacter(
        EditorStore &store, log::ILogger &logger);

    /**
     * @brief The source rectangle of one animation frame.
     */
    [[nodiscard]] gfx::RectF characterFrameSource(
        std::int32_t row, std::int32_t frame) noexcept;

    /**
     * @brief Draws the magnified character workspace.
     *
     * Mirrors the tile workspace presentation with 16x16 frame
     * guides, and adds the animated row preview in the left margin.
     */
    void drawCharacterWorkspace(
        gfx::ViewportRenderer &view,
        const gfx::ITexture &sheet,
        std::optional<gfx::Point> hover,
        std::uint32_t tick);

}
