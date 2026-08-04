#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    /**
     * @brief What every state dump document says it is.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-game-state-dump";

    /**
     * @brief The dump revision this build writes.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 1;

    /**
     * @brief The running session, as the console's dump_state takes it.
     *
     * **A save plus the view around it.** A SaveGame already carries
     * everything the picker persists -- the grid, the walkers, the
     * buildings, the camera, the money and the seed -- and this wraps
     * it with the rest of what a click's meaning depends on: the
     * pause, the selected tool, the map view, the language and the
     * console itself.
     * Coming back to a dump therefore means coming back to the
     * instant it was taken, not merely to its city.
     *
     * What it deliberately does not carry: the engine's tick number,
     * which a recording forbids going backwards; the other cities of
     * the world map, which a save has never carried either; and any
     * open menu, which load_state closes exactly as opening the menu
     * modal ends a road drag.
     */
    struct StateDump
    {
        /** @brief The city, exactly as a save file holds one. */
        SaveGame save;

        /** @brief Whether the run was held still. */
        bool paused = false;

        /**
         * @brief What a left click was placing, if anything.
         *
         * Absent means the palette was down, which is a state of its
         * own rather than a synonym for any tool -- see UiOverlay.
         */
        std::optional<BuildTool> tool = std::nullopt;

        /** @brief Which picture of the city was showing. */
        MapView view = MapView::Normal;

        /** @brief The language the run was worded in. */
        antwika::i18n::Locale locale = antwika::i18n::kDefaultLocale;

        /** @brief The console's history, oldest line first. */
        std::vector<std::string> console;

        /**
         * @brief Compare two dumps.
         * @param other The dump to compare against.
         * @return True when every field matches.
         */
        [[nodiscard]] bool operator==(
            const StateDump &other) const = default;
    };

    /**
     * @brief Build the chain that brings an old dump document up.
     *
     * Empty at version 1, and a factory anyway, so adding the first
     * migration changes this function and nothing else -- the same
     * shape standardOptionsMigrations() has.
     *
     * @return The chain, currently with no steps.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump as JSON matching the dump-document schema.
     *
     * Pure: no filesystem, no clock. The embedded save is a complete
     * versioned save document of its own, so the two formats migrate
     * independently -- a dump written today still reads after the
     * save format's next bump, through the save's own chain.
     *
     * @param dump The state to encode.
     * @return The encoded document, carrying kStateDumpVersion.
     */
    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    /**
     * @brief Decode a dump from JSON, validating it first.
     *
     * The same four stages every persisted document here goes
     * through: read the version, migrate up, validate, decode -- see
     * docs/schema-versioning.md.
     *
     * @param document The document to read.
     * @return The decoded state.
     * @throws SaveFormatError If the document is not a state dump
     * this build can read.
     */
    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &document);

} // namespace antwika::game
