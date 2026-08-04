#pragma once

#include <cstdint>
#include <istream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/MigrationChain.hpp>

#include "antwika/game/KeyBindings.hpp"

namespace antwika::game
{

    using antwika::replay::MigrationChain;

    /**
     * @brief Everything an options file holds.
     *
     * Two preferences that have nothing to do with each other beyond
     * both being the player's rather than the session's, which is what
     * one file is for. They travel together because they are read and
     * written together; neither knows about the other.
     *
     * **Neither is simulation state as it sits here.** The bindings
     * become some through OptionsState and the language through
     * LocaleState, and both reach a run the same way: announced once,
     * ahead of the first tick, by a source that a replay constructs
     * empty -- see BindingSource and LocaleSource.
     */
    struct PlayerOptions
    {
        /**
         * @brief Which key asks for what.
         */
        KeyBindings bindings{};

        /**
         * @brief Which language the captions are worded in.
         */
        antwika::i18n::Locale locale{antwika::i18n::kDefaultLocale};

        /**
         * @brief Compare two sets of preferences.
         * @param other The preferences to compare against.
         * @return True when both members match.
         */
        [[nodiscard]] bool operator==(
            const PlayerOptions &other) const = default;
    };

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
    inline constexpr std::uint32_t kOptionsFormatVersion = 2;

    /**
     * @brief The member an options document names its language in.
     *
     * Written once so the schema, the migration, the encoder and the
     * decoder cannot disagree about it -- four places that each spelled
     * it out would be four chances to write one of them differently.
     */
    inline constexpr std::string_view kLocaleKey = "locale";

    /**
     * @brief Build the migration chain for the options document format.
     * @return A chain that brings an options document of any version
     * this build still reads up to kOptionsFormatVersion.
     *
     * One step long: version 2 added the picked language, so a
     * version 1 document has "locale" written into it as the default
     * language's tag before it is validated. That is the whole reason
     * the chain was built empty rather than left out -- the reading
     * order is `parse -> read version -> migrate -> validate -> decode`
     * whether or not there is a step to take, so adding the first real
     * step changed one function and no caller.
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
    [[nodiscard]] nlohmann::json optionsToJson(
        const PlayerOptions &options);

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
    [[nodiscard]] PlayerOptions optionsFromJson(
        const nlohmann::json &document);

    /**
     * @brief Write a layout to a stream.
     * @param bindings The layout to write.
     * @param out Receives the document.
     */
    void writeOptions(const PlayerOptions &options, std::ostream &out);

    /**
     * @brief Read a layout from a stream.
     * @param in Holds the document.
     * @return The layout it holds.
     * @throws OptionsFormatError If the stream does not hold one.
     */
    [[nodiscard]] PlayerOptions readOptions(std::istream &in);

    /**
     * @brief Write a layout to a file.
     * @param bindings The layout to write.
     * @param path Where to write it.
     * @throws OptionsFormatError If the file cannot be opened or
     * written.
     */
    void saveOptionsFile(
        const PlayerOptions &options, const std::string &path);

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
    [[nodiscard]] PlayerOptions loadOptionsFileOrDefaults(
        const std::string &path);

    /**
     * @brief Write a layout, if anywhere was named.
     * @param bindings The layout to write.
     * @param path Where to write it, or nothing to write nowhere.
     * @throws OptionsFormatError If the file cannot be opened or
     * written.
     */
    void saveOptionsFileIfNamed(
        const PlayerOptions &options,
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
         * @brief The language the machine is carrying, if it is this
         * run's business at all.
         *
         * Empty for a replay for the reason the bindings are: the
         * recording already carries the language it was played in, and
         * reading the machine's over the top of it would resolve
         * recorded clicks against a layout the recording never had.
         */
        std::optional<antwika::i18n::Locale> locale{};

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
