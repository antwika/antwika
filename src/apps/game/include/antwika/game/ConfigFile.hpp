#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/GameConfig.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    /**
     * @brief What every document of this format says it is.
     *
     * Checked before anything else is read, so a save, a replay or an
     * options file handed to this loader is refused as the wrong kind
     * of file rather than as a config with every member missing -- all
     * of these formats state their version in the same member, so the
     * magic is the only thing telling them apart.
     */
    inline constexpr std::string_view kConfigMagic =
        "antwika-game-config";

    /**
     * @brief Which revision of the config format this build writes.
     *
     * Stated in antwika::replay::kSchemaVersionKey -- "version" -- the
     * one member every persisted document in this code base carries its
     * version in, rather than a name of this format's own.
     */
    inline constexpr std::uint32_t kConfigFormatVersion = 1;

    /**
     * @brief Build the migration chain for the config document format.
     * @return A chain that brings a config document of any version this
     * build still reads up to kConfigFormatVersion.
     *
     * Empty today, because there has only ever been one revision, and
     * present anyway: the reading order is `parse -> read version ->
     * migrate -> validate -> decode` whether or not there is a step to
     * take, and a chain constructed here is what refuses a document
     * from a newer build instead of decoding it on the strength of
     * happening to satisfy today's schema.
     */
    [[nodiscard]] MigrationChain standardConfigMigrations();

    /**
     * @brief Encode a config as a config document.
     * @param config The config to write.
     * @return The document, stating its magic, its version and every
     * member outright -- a written config is the full picture, so what
     * each number is called and currently is can be read off the file.
     */
    [[nodiscard]] nlohmann::json configToJson(const GameConfig &config);

    /**
     * @brief Decode a config document.
     *
     * Every member but the magic is optional, and an absent one means
     * the shipped default rather than an error: a config stating one
     * number is a one-line rebalance, not a restatement of every
     * default it leaves alone. That also makes adding a field
     * additive -- an older file simply does not state it -- which is
     * why this format should stay at version 1 for as long as no
     * member changes meaning.
     *
     * @param document The parsed document.
     * @return The config it states, defaults filling what it does not.
     * @throws ConfigFormatError If it is not this format, states a
     * version this build cannot reach the current one from, or fails
     * the schema -- a member of the wrong shape, a period of zero
     * ticks, a negative cost, or a building kind no name goes by.
     */
    [[nodiscard]] GameConfig configFromJson(const nlohmann::json &document);

    /**
     * @brief Write a config to a stream.
     * @param config The config to write.
     * @param out Receives the document.
     */
    void writeConfig(const GameConfig &config, std::ostream &out);

    /**
     * @brief Read a config from a stream.
     * @param in Holds the document.
     * @return The config it holds.
     * @throws ConfigFormatError If the stream does not hold one.
     */
    [[nodiscard]] GameConfig readConfig(std::istream &in);

    /**
     * @brief Read the config the game ships beside its assets.
     *
     * **A missing file is an ordinary install**, not an error: a build
     * nobody has rebalanced plays the game these sources define, which
     * is exactly what a default-constructed GameConfig is. Anything else
     * wrong with a file that is there is refused rather than repaired,
     * for the reason ConfigFormatError gives.
     *
     * @param path Where the file would be.
     * @return What it held, or the defaults when it is not there.
     * @throws ConfigFormatError If a file is there and is not one of
     * these.
     */
    [[nodiscard]] GameConfig loadConfigFileOrDefaults(
        const std::string &path);

} // namespace antwika::game
