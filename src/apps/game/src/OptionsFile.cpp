#include "antwika/game/OptionsFile.hpp"

#include <fstream>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/config/ConfigDocument.hpp>
#include <antwika/config/FileFormat.hpp>
#include <antwika/config/Format.hpp>
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
        using OptionsFormat = FileFormat<KeyBindings, OptionsFormatError>;

        // Two spaces, one member a line.
        // Enough to diff a layout against the next version of itself.
        constexpr int kIndent = 2;

        void describeMembers(nlohmann::json &schema)
        {
            nlohmann::json binding;
            binding["type"] = "object";
            binding["additionalProperties"] = false;
            binding["required"] = {"action", "key"}; // GCOVR_EXCL_LINE
            binding["properties"]["action"]["type"] = "string";
            binding["properties"]["key"]["type"] = "string";

            schema["required"] = {
                "magic", "bindings"}; // GCOVR_EXCL_LINE
            schema["properties"]["bindings"]["type"] = "array";
            schema["properties"]["bindings"]["items"] = binding;
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

    MigrationChain standardOptionsMigrations()
    {
        // Every branch left on the excluded line is the allocator's.
        // The list is empty, so no heap branch is taken here.
        // What is left is the throw edge of building it.
        return MigrationChain({}, kOptionsFormatVersion); // GCOVR_EXCL_LINE
    }

    namespace
    {
        void encodeMembers(
            const KeyBindings &bindings, nlohmann::json &encoded)
        {
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

        KeyBindings decodeMembers(const nlohmann::json &document)
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

            return bindings;
        }

        const OptionsFormat &optionsFormat()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            // See docs/confirming-unreachable-branches.md.
            static const OptionsFormat format(
                FormatSpec<KeyBindings>{
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

    nlohmann::json bindingsToJson(const KeyBindings &bindings)
    {
        return optionsFormat().toJson(bindings);
    }

    KeyBindings bindingsFromJson(const nlohmann::json &document)
    {
        return optionsFormat().fromJson(document);
    }

    void writeOptions(const KeyBindings &bindings, std::ostream &out)
    {
        optionsFormat().write(bindings, out);
    }

    KeyBindings readOptions(std::istream &in)
    {
        return optionsFormat().read(in);
    }

    void saveOptionsFile(
        const KeyBindings &bindings, const std::string &path)
    {
        std::ofstream file(path);
        if (!file.is_open())
        {
            throw OptionsFormatError(
                "antwika::game: could not open options to write: "
                + path);
        }

        writeOptions(bindings, file);

        // Flushed here rather than by the destructor, which cannot say.
        // A full disk fails on the flush, not on the open.
        file.flush();
        if (!file)
        {
            throw OptionsFormatError(
                "antwika::game: could not write options: " + path);
        }
    }

    KeyBindings loadOptionsFileOrDefaults(const std::string &path)
    {
        std::ifstream file(path);

        // A file that is not there is a player who never opened this.
        // Which is a state rather than a failure.
        if (!file.is_open())
        {
            return kDefaultBindings;
        }

        return readOptions(file);
    }

    void saveOptionsFileIfNamed(
        const KeyBindings &bindings,
        const std::optional<std::string> &path)
    {
        if (!path.has_value())
        {
            return;
        }

        saveOptionsFile(bindings, *path);
    }

    MachineOptions machineOptionsFor(
        bool replaying, const std::string &path)
    {
        if (replaying)
        {
            return MachineOptions{};
        }

        return MachineOptions{
            .bindings = loadOptionsFileOrDefaults(path), .path = path};
    }

} // namespace antwika::game
