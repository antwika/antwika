#pragma once

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/KeyBindings.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    /**
     * @brief What every document of this format says it is.
     *
     * Checked before anything else is read, so a save or a replay handed
     * to this loader is refused as the wrong kind of file rather than as
     * an options document with every member missing -- both formats
     * state their version in the same member, so the magic is the only
     * thing telling them apart.
     */
    inline constexpr std::string_view kOptionsMagic =
        "antwika-game-options";

    /**
     * @brief Which revision of the options format this build writes.
     *
     * Stated in antwika::replay::kSchemaVersionKey -- "version" -- the
     * one member every persisted document in this code base carries its
     * version in, rather than a name of this format's own.
     */
    inline constexpr std::uint32_t kOptionsFormatVersion = 1;

    /**
     * @brief Build the migration chain for the options document format.
     * @return A chain that brings an options document of any version
     * this build still reads up to kOptionsFormatVersion.
     *
     * Empty today, because there has only ever been one revision, and
     * present anyway: the reading order is `parse -> read version ->
     * migrate -> validate -> decode` whether or not there is a step to
     * take, and a chain constructed here is what refuses a document from
     * a newer build instead of decoding it on the strength of happening
     * to satisfy today's schema.
     * A factory rather than a constant, so adding a migration changes
     * one function; list order is not semantic, so appending to it is
     * conflict-free.
     */
    [[nodiscard]] MigrationChain standardOptionsMigrations();

    /**
     * @brief Encode a layout as an options document.
     * @param bindings The layout to write.
     * @return The document, stating its magic and its version.
     */
    [[nodiscard]] nlohmann::json bindingsToJson(
        const KeyBindings &bindings);

    /**
     * @brief Decode an options document.
     * @param document The parsed document.
     * @return The layout it holds.
     * @throws OptionsFormatError If it is not this format, states a
     * version this build cannot reach the current one from, fails the
     * schema, names an action or a key this build does not know, or
     * describes a layout that cannot exist -- two actions on one key, or
     * an action on a key this application spends above the tick loop.
     */
    [[nodiscard]] KeyBindings bindingsFromJson(
        const nlohmann::json &document);

    /**
     * @brief Write a layout to a stream.
     * @param bindings The layout to write.
     * @param out Receives the document.
     */
    void writeOptions(const KeyBindings &bindings, std::ostream &out);

    /**
     * @brief Read a layout from a stream.
     * @param in Holds the document.
     * @return The layout it holds.
     * @throws OptionsFormatError If the stream does not hold one.
     */
    [[nodiscard]] KeyBindings readOptions(std::istream &in);

    /**
     * @brief Write a layout to a file.
     * @param bindings The layout to write.
     * @param path Where to write it.
     * @throws OptionsFormatError If the file cannot be opened or
     * written.
     */
    void saveOptionsFile(
        const KeyBindings &bindings, const std::string &path);

    /**
     * @brief Read the layout a machine is carrying.
     *
     * **A missing file is an ordinary first run**, not an error: a
     * player who has never opened the options screen is playing the
     * layout this application ships, which is exactly what
     * kDefaultBindings is. Anything else wrong with the file is refused
     * rather than repaired, for SaveGame's reason -- a repaired
     * options file is a layout somebody never chose.
     *
     * @param path Where the file would be.
     * @return What it held, or kDefaultBindings when it is not there.
     * @throws OptionsFormatError If a file is there and is not one of
     * these.
     */
    [[nodiscard]] KeyBindings loadOptionsFileOrDefaults(
        const std::string &path);

    /**
     * @brief Write a layout, if anywhere was named.
     * @param bindings The layout to write.
     * @param path Where to write it, or nothing to write nowhere.
     * @throws OptionsFormatError If the file cannot be opened or
     * written.
     */
    void saveOptionsFileIfNamed(
        const KeyBindings &bindings,
        const std::optional<std::string> &path);

    /**
     * @brief What a run makes of the machine it is running on.
     *
     * Both members are absent for a replay, and that is the whole point
     * of the type: **a replay neither reads the machine's layout nor
     * writes to it.** Reading it would resolve a recorded key press
     * against a layout the recording never carried; writing it would
     * leave whoever played somebody else's recording with that
     * session's bindings.
     */
    struct MachineOptions
    {
        /**
         * @brief The layout the machine is carrying, if it is this run's
         * business at all.
         */
        std::optional<KeyBindings> bindings{};

        /**
         * @brief Where this run should leave its layout, if anywhere.
         */
        std::optional<std::string> path{};

        /**
         * @brief Compare two answers.
         * @param other The answer to compare against.
         * @return True when both members match.
         */
        [[nodiscard]] bool operator==(
            const MachineOptions &other) const = default;
    };

    /**
     * @brief Decide what a run has to do with the machine's own layout.
     *
     * It lives here rather than in a main() so the coverage report sees
     * it: a composition root is the one file that is not measured, and a
     * decision about whether a replay may read a file is exactly what
     * that exclusion must not be hiding.
     *
     * @param replaying Whether this run's input comes from a file.
     * @param path Where the machine keeps its layout.
     * @return Nothing at all for a replay; otherwise the layout that
     * path holds and that path to write back to.
     * @throws OptionsFormatError If a file is there and is not one of
     * these.
     */
    [[nodiscard]] MachineOptions machineOptionsFor(
        bool replaying, const std::string &path);

} // namespace antwika::game
