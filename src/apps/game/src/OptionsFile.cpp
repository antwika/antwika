#include "antwika/game/OptionsFile.hpp"

#include <fstream>
#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/replay/SchemaVersion.hpp>
#include <antwika/replay/VersionedDocument.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        // Two spaces, one member a line.
        // Enough to diff a layout against the next version of itself.
        constexpr int kIndent = 2;

        nlohmann::json optionsSchema()
        {
            nlohmann::json binding;
            binding["type"] = "object";
            binding["additionalProperties"] = false;
            binding["required"] = {"action", "key"}; // GCOVR_EXCL_LINE
            binding["properties"]["action"]["type"] = "string";
            binding["properties"]["key"]["type"] = "string";

            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika game options document";
            schema["type"] = "object";
            schema["additionalProperties"] = false;

            // The version member is described but not required.
            // A document without one is read as version 1 instead.
            // By the time this runs the document has been migrated.
            // So the only version it may carry is the current one.
            schema["required"] = {"magic", "bindings"}; // GCOVR_EXCL_LINE
            schema["properties"]["magic"]["const"] =
                std::string(kOptionsMagic);
            schema["properties"][std::string(replay::kSchemaVersionKey)]
                  ["const"] = kOptionsFormatVersion;
            schema["properties"]["bindings"]["type"] = "array";
            schema["properties"]["bindings"]["items"] = binding;
            return schema;
        }

        const nlohmann::json_schema::json_validator &optionsValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                optionsSchema()); // GCOVR_EXCL_LINE
            return validator;
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

    nlohmann::json bindingsToJson(const KeyBindings &bindings)
    {
        nlohmann::json encoded;
        encoded["magic"] = std::string(kOptionsMagic);
        encoded[std::string(replay::kSchemaVersionKey)] =
            kOptionsFormatVersion;
        encoded["bindings"] = nlohmann::json::array();

        for (const auto action : kActions)
        {
            nlohmann::json one;
            one["action"] = std::string(actionName(action));
            one["key"] = std::string(
                antwika::input::toString(bindings.keyFor(action)));
            encoded["bindings"].push_back(std::move(one));
        }

        return encoded;

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    KeyBindings bindingsFromJson(const nlohmann::json &document)
    {
        const auto migrated =
            replay::readVersionedDocument<OptionsFormatError>(
                document,
                standardOptionsMigrations(),
                optionsValidator(),
                "antwika::game: options JSON failed schema validation: ");

        KeyBindings bindings;

        for (const auto &one : migrated.at("bindings"))
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

    void writeOptions(const KeyBindings &bindings, std::ostream &out)
    {
        out << bindingsToJson(bindings).dump(kIndent) << '\n';
    }

    KeyBindings readOptions(std::istream &in)
    {
        nlohmann::json document;
        try
        {
            in >> document;
        }
        catch (const nlohmann::json::exception &error) // GCOVR_EXCL_LINE
        {
            throw OptionsFormatError(
                std::string(
                    "antwika::game: options are not valid JSON: ")
                + error.what());
        }

        return bindingsFromJson(document);
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
