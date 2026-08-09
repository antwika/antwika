#include "antwika/game/OptionsFile.hpp"

#include <nlohmann/json-schema.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/io/File.hpp>
#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
#include <antwika/replay/IMigration.hpp>
#include <antwika/replay/JsonShapes.hpp>
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

        using OptionsFormat = FileFormat<PlayerOptions, OptionsFormatError>;

        void describeMembers(nlohmann::json &schema)
        {
            nlohmann::json binding =
                antwika::replay::objectShape({"action", "key"});
            binding["properties"]["action"] =
                antwika::replay::wordShape();
            binding["properties"]["key"] = antwika::replay::wordShape();

            schema["required"] = antwika::replay::requiredShape(
                {"magic", "bindings", "locale", "keyboard"});
            schema["properties"]["bindings"]["type"] = "array";
            schema["properties"]["bindings"]["items"] = binding;
            schema["properties"][std::string(kLocaleKey)] =
                antwika::replay::wordShape();
            schema["properties"][std::string(kKeyboardKey)] =
                antwika::replay::wordShape();
        }

        Key keyNamed(const std::string &name)
        {
            try
            {
                return antwika::input::keyFromString(name);
            }
            catch (const antwika::input::InputError &error) // GCOVR_EXCL_LINE
            {
                throw OptionsFormatError(error.what());
            }
        }
    }

    namespace
    {
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
    }

    namespace
    {
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
    }

    MigrationChain standardOptionsMigrations()
    {
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
    }

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
        io::writeFileAs<OptionsFormatError>(
            path, "options", [&options](std::ostream &out) {
                writeOptions(options, out);
            });
    }

    PlayerOptions loadOptionsFileOrDefaults(const std::string &path)
    {
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

}
