#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/sudoku/SudokuConfig.hpp"

namespace antwika::sudoku
{

    using antwika::replay::MigrationChain;

    /**
     * @brief What every document of this format says it is.
     *
     * The magic is the one thing telling this format's file apart
     * from every other persisted document -- they all state their
     * version in the same member.
     */
    inline constexpr std::string_view kConfigMagic =
        "antwika-sudoku-config";

    /** @brief Which revision of the format this build writes. */
    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    /**
     * @brief Build the migration chain for the config document.
     * @return A chain up to kConfigFormatVersion; empty today, and
     * present anyway, since it is what refuses a document from a
     * newer build.
     */
    [[nodiscard]] MigrationChain standardConfigMigrations();

    /**
     * @brief Encode a config as a document stating every member.
     * @param config The config to write.
     * @return The document.
     */
    [[nodiscard]] nlohmann::json configToJson(const SudokuConfig &config);

    /**
     * @brief Decode a config document.
     *
     * Every member but the magic is optional; an absent one means
     * the shipped default, so a config stating one number is a
     * one-line change.
     *
     * @param document The parsed document.
     * @return The config it states, defaults filling the rest.
     * @throws antwika::config::ConfigFormatError If it is not this
     * format, states a version this build cannot reach the current
     * one from, or fails the schema.
     */
    [[nodiscard]] SudokuConfig configFromJson(
        const nlohmann::json &document);

    /**
     * @brief Write a config to a stream.
     * @param config The config to write.
     * @param out Receives the document.
     */
    void writeConfig(const SudokuConfig &config, std::ostream &out);

    /**
     * @brief Read a config from a stream.
     * @param in Holds the document.
     * @return The config it holds.
     * @throws antwika::config::ConfigFormatError If the stream does
     * not hold one.
     */
    [[nodiscard]] SudokuConfig readConfig(std::istream &in);

    /**
     * @brief Read the config this application ships beside its
     * assets.
     *
     * A missing file is an ordinary install playing the shipped
     * defaults; anything wrong with a file that is there is refused
     * rather than repaired.
     *
     * @param path Where the file would be.
     * @return What it held, or the defaults when it is not there.
     * @throws antwika::config::ConfigFormatError If a file is there
     * and is not one of these.
     */
    [[nodiscard]] SudokuConfig loadConfigFileOrDefaults(
        const std::string &path);

} // namespace antwika::sudoku
