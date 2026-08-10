#pragma once

#include <nlohmann/json_fwd.hpp>

#include <array>
#include <filesystem>
#include <optional>
#include <string>

#include <antwika/enums/Enumeration.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/tilemap/TerrainClass.hpp>

namespace antwika::map_editor
{

    inline constexpr std::size_t kTerrainCount =
        enums::kCount<tilemap::TerrainClass>;

    struct GenerationRules final
    {
        std::array<double, kTerrainCount> weights{
            8.0, 3.0, 2.0, 1.0, 2.0, 1.0};
        std::array<std::array<bool, kTerrainCount>, kTerrainCount>
            allowed{};

        [[nodiscard]] bool operator==(
            const GenerationRules &other) const = default;
    };

    /**
     * @brief The compiled-in rules matching the shipped defaults.
     *
     * Ensures: the adjacency matrix is symmetric and matches the
     *          task 3 convention, and the weights are the 8/3/2/2/1
     *          set with stair fixed at one.
     */
    [[nodiscard]] GenerationRules defaultGenerationRules();

    /**
     * @brief Parses a rules document.
     *
     * @return The rules, or nothing when a terrain name is unknown,
     *         a weight is not a positive number, or an adjacency
     *         entry is not a two-name pair.
     *
     * Ensures: every accepted pair is applied symmetrically, and
     *          the stair weight stays at its default.
     */
    [[nodiscard]] std::optional<GenerationRules> rulesFromJson(
        const nlohmann::json &document);

    [[nodiscard]] nlohmann::json rulesToJson(
        const GenerationRules &rules);

    /**
     * @brief Loads the rules file beside the tile sheets.
     *
     * Ensures: a missing or corrupt file logs a warning and yields
     *          the compiled-in defaults.
     */
    [[nodiscard]] GenerationRules loadRulesFileOrDefaults(
        const std::filesystem::path &path, log::ILogger &logger);

    /**
     * @brief Writes the rules file.
     *
     * @return An error message, or nothing on success.
     */
    [[nodiscard]] std::optional<std::string> saveRulesFile(
        const std::filesystem::path &path,
        const GenerationRules &rules);

}
