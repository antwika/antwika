#include "antwika/game/OptionsFile.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <nlohmann/json-schema.hpp>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/io/File.hpp>

#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        using antwika::config::FileFormat;
        using antwika::config::FormatSpec;

        // An options file is a versioned JSON document like any other.
        // It reports an OptionsFormatError while being read.
        // Which is why the format is templated on its error type.
        using OptionsFormat = FileFormat<PlayerOptions, OptionsFormatError>;

        void describeMembers(nlohmann::json &schema)
        {
            nlohmann::json binding;
            binding["type"] = "object";
            binding["additionalProperties"] = false;
            binding["required"] = {"action", "key"}; // GCOVR_EXCL_LINE
            binding["properties"]["action"]["type"] = "string";
            binding["properties"]["key"]["type"] = "string";

            schema["required"] = {
                "magic",
                "bindings",
                "locale",
                "keyboard"}; // GCOVR_EXCL_LINE
            schema["properties"]["bindings"]["type"] = "array";
            schema["properties"]["bindings"]["items"] = binding;
            schema["properties"][std::string(kLocaleKey)]["type"]
                = "string";
            schema["properties"][std::string(kKeyboardKey)]["type"]
                = "string";
        }

        // The key's own name is antwika::input's to police.
        // Its error may not leave a call promising this one's.
        // So it is carried through here rather than rewritten.
        Key keyNamed(const std::string &name)
        {
            try
            {
                return antwika::input::keyFromString(name);
            }
            // The handler's own no-match edge is unreachable.
            // Only an InputError can arrive, and only from above.
            // PayloadJson.hpp marks its own catch for the same reason.
            catch (const antwika::input::InputError &error) // GCOVR_EXCL_LINE
            {
                throw OptionsFormatError(error.what());
            }
        }
    } // namespace

    namespace
    {
        // Version 2 added the language the captions are worded in.
        // A version 1 document was written before there was a choice.
        // So it played in the default one, and now says so.
        // Written before validation rather than after it.
        // Which is what lets the schema require the member.
        class OptionsV1ToV2 final : public antwika::replay::IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept
                override
            {
                return 1;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return 2;
            }

            // Only ever read to word a MigrationChain's refusal.
            // It is the message thrown when a step is not one step.
            // This one reads 1 and produces 2.
            // So reaching it means editing the two functions above.
            // Which breaks the migration rather than feeding it input.
            // See docs/confirming-unreachable-branches.md.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "options v1 -> v2: the picked language";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                document[std::string(kLocaleKey)] = std::string(
                    antwika::i18n::tagOf(antwika::i18n::kDefaultLocale));
            }
        };
    } // namespace

    namespace
    {
        // Version 3 added the board the typed characters come off.
        // A version 2 document was written before there was a choice.
        // Every run before it typed by the shipped default's table.
        // So it says the default now, and validates like a new one.
        class OptionsV2ToV3 final : public antwika::replay::IMigration
        {
        public:
            [[nodiscard]] std::uint32_t fromVersion() const noexcept
                override
            {
                return 2;
            }

            [[nodiscard]] std::uint32_t toVersion() const noexcept
                override
            {
                return 3;
            }

            // Only ever read to word a MigrationChain's refusal.
            // OptionsV1ToV2 says why no input can reach it.
            // GCOVR_EXCL_START
            [[nodiscard]] std::string_view name() const noexcept override
            {
                return "options v2 -> v3: the keyboard layout";
            }
            // GCOVR_EXCL_STOP

            void apply(nlohmann::json &document) const override
            {
                document[std::string(kKeyboardKey)] = std::string(
                    keyboardLayoutName(kDefaultKeyboardLayout));
            }
        };
    } // namespace

    MigrationChain standardOptionsMigrations()
    {
        // Every branch left on the excluded lines is the allocator's.
        // The unwind edges of building a list of two shared_ptrs.
        antwika::replay::MigrationList migrations;
        migrations.push_back(
            std::make_shared<const OptionsV1ToV2>()); // GCOVR_EXCL_LINE
        migrations.push_back(
            std::make_shared<const OptionsV2ToV3>()); // GCOVR_EXCL_LINE

        return MigrationChain(
            std::move(migrations),
            kOptionsFormatVersion); // GCOVR_EXCL_LINE
    }

    namespace
    {
        void encodeMembers(
            const PlayerOptions &options, nlohmann::json &encoded)
        {
            const KeyBindings &bindings = options.bindings;

            encoded[std::string(kLocaleKey)] =
                std::string(antwika::i18n::tagOf(options.locale));
            encoded[std::string(kKeyboardKey)] =
                std::string(keyboardLayoutName(options.keyboard));
            encoded["bindings"] = nlohmann::json::array();

            for (const auto action : kActions)
            {
                nlohmann::json one;
                one["action"] = std::string(actionName(action));
                one["key"] = std::string(
                    antwika::input::toString(bindings.keyFor(action)));
                encoded["bindings"].push_back(std::move(one));
            }
        }

        PlayerOptions decodeMembers(const nlohmann::json &document)
        {
            KeyBindings bindings;

            for (const auto &one : document.at("bindings"))
            {
                const auto name = one.at("action").get<std::string>();
                const auto action = actionFromName(name);

                if (!action.has_value())
                {
                    throw OptionsFormatError(
                        "antwika::game: options name an action this build "
                        "does not know: "
                        + name);
                }

                const auto named = one.at("key").get<std::string>();
                const auto outcome = bindings.bind(*action, keyNamed(named));

                // Refused rather than repaired.
                // Exactly as a save's building/walker link is.
                // A layout nobody could have chosen is not one to guess at.
                if (outcome == BindOutcome::Reserved)
                {
                    throw OptionsFormatError(
                        "antwika::game: options bind a key this build acts "
                        "on above the tick loop: "
                        + named);
                }

                if (outcome == BindOutcome::Taken)
                {
                    throw OptionsFormatError(
                        "antwika::game: options bind two actions to one "
                        "key: "
                        + named);
                }
            }

            // The tag is the migration's business to have put there.
            // A version 1 document reaches this with the default's.
            // So this lookup answers for every document that validates.
            const auto tag =
                document.at(std::string(kLocaleKey)).get<std::string>();
            const auto locale = antwika::i18n::localeFromTag(tag);

            if (!locale.has_value())
            {
                throw OptionsFormatError(
                    "antwika::game: options name a language this build "
                    "has no catalogue for: "
                    + tag);
            }

            const auto board =
                document.at(std::string(kKeyboardKey))
                    .get<std::string>();
            const auto keyboard = keyboardLayoutFromName(board);

            if (!keyboard.has_value())
            {
                throw OptionsFormatError(
                    "antwika::game: options name a keyboard layout "
                    "this build does not know: "
                    + board);
            }

            return PlayerOptions{
                .bindings = bindings,
                .locale = *locale,
                .keyboard = *keyboard};
        }

        const OptionsFormat &optionsFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const OptionsFormat format(
                FormatSpec<PlayerOptions>{
                    .format =
                        {.magic = kOptionsMagic,
                         .version = kOptionsFormatVersion},
                    .title = "antwika game options document",
                    .whatFailed = "antwika::game: options JSON failed "
                                  "schema validation: ",
                    .members = describeMembers,
                    .encode = encodeMembers,
                    .decode = decodeMembers,
                    .migrations =
                        standardOptionsMigrations}); // GCOVR_EXCL_LINE
            return format;
        }
    } // namespace

    nlohmann::json optionsToJson(const PlayerOptions &options)
    {
        return optionsFormat().toJson(options);
    }

    PlayerOptions optionsFromJson(const nlohmann::json &document)
    {
        return optionsFormat().fromJson(document);
    }

    void writeOptions(const PlayerOptions &options, std::ostream &out)
    {
        optionsFormat().write(options, out);
    }

    PlayerOptions readOptions(std::istream &in)
    {
        return optionsFormat().read(in);
    }

    void saveOptionsFile(
        const PlayerOptions &options, const std::string &path)
    {
        // The open, the flush and the write refusal are antwika::io's.
        // That discipline is stated once, over there.
        io::writeFileAs<OptionsFormatError>(
            path, "options", [&options](std::ostream &out) {
                writeOptions(options, out);
            });
    }

    PlayerOptions loadOptionsFileOrDefaults(const std::string &path)
    {
        // A file that is not there is a player who never opened this.
        // Which is a state rather than a failure.
        auto file = io::openToReadIfPresent(path);

        if (!file.has_value())
        {
            return PlayerOptions{};
        }

        return readOptions(*file);
    }

    void saveOptionsFileIfNamed(
        const PlayerOptions &options,
        const std::optional<std::string> &path)
    {
        if (!path.has_value())
        {
            return;
        }

        saveOptionsFile(options, *path);
    }

    MachineOptions machineOptionsFor(
        bool replaying, const std::string &path)
    {
        if (replaying)
        {
            return MachineOptions{};
        }

        const auto stored = loadOptionsFileOrDefaults(path);

        return MachineOptions{
            .bindings = stored.bindings,
            .locale = stored.locale,
            .keyboard = stored.keyboard,
            .path = path};
    }

} // namespace antwika::game
