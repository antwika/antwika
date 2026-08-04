#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/BuildTool.hpp"
#include "antwika/game/MapView.hpp"
#include "antwika/game/SaveGame.hpp"

namespace antwika::game
{

    /**
     * @brief What every dump of this application says it is.
     */
    inline constexpr std::string_view kStateDumpMagic =
        "antwika-game-state-dump";

    /**
     * @brief The dump revision this build writes.
     *
     * Version 2 moved the state under console::SnapshotFormat's shared
     * envelope, with the console's history beside it; version 1 was
     * this application's own bespoke document.
     */
    inline constexpr std::uint32_t kStateDumpVersion = 2;

    /**
     * @brief The running session, as the console's dump_state takes it.
     *
     * **A save plus the view around it.** A SaveGame already carries
     * everything the picker persists -- the grid, the walkers, the
     * buildings, the camera, the money and the seed -- and this wraps
     * it with the rest of what a click's meaning depends on: the
     * pause, the selected tool, the map view and the language.
     * Coming back to a dump therefore means coming back to the
     * instant it was taken, not merely to its city.
     *
     * The console's own history rides in the envelope rather than in
     * here, since carrying the console is every application's dump
     * behaviour and written once -- see console::SnapshotFormat.
     *
     * What it deliberately does not carry: the engine's tick number,
     * which a recording forbids going backwards; the other cities of
     * the world map, which a save has never carried either; any open
     * menu, which load_state closes exactly as opening the menu modal
     * ends a road drag; and the machine's own key bindings and
     * keyboard layout, which are options rather than the session.
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
     * One step: a version 1 document was this application's own
     * bespoke shape, and moving its members under the envelope's
     * "state" object is the whole of what version 2 changed.
     *
     * @return The chain.
     */
    [[nodiscard]] antwika::replay::MigrationChain
    standardStateDumpMigrations();

    /**
     * @brief Encode a dump as the envelope's opaque state object.
     *
     * Pure: no filesystem, no clock. The embedded save is a complete
     * versioned save document of its own, so the two formats migrate
     * independently.
     *
     * @param dump The state to encode.
     * @return The state object, magic-free: the envelope stamps the
     * document -- see console::SnapshotFormat.
     */
    [[nodiscard]] nlohmann::json stateDumpToJson(const StateDump &dump);

    /**
     * @brief Decode the envelope's state object, validating it first.
     * @param state The state object a snapshot carried.
     * @return The decoded state.
     * @throws SaveFormatError If the object is not a state this build
     * can read.
     */
    [[nodiscard]] StateDump stateDumpFromJson(
        const nlohmann::json &state);

} // namespace antwika::game
