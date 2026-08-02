#include "antwika/game/BindingEvent.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json bindKeySchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "game.bind_key payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"action", "key"}; // GCOVR_EXCL_LINE
            schema["properties"]["action"]["type"] = "string";
            schema["properties"]["key"]["type"] = "string";
            return schema;
        }

        const nlohmann::json_schema::json_validator &bindKeyValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                bindKeySchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    std::string bindKeyPayload(KeyBinding binding)
    {
        nlohmann::json payload;
        payload["action"] = std::string(actionName(binding.action));
        payload["key"] =
            std::string(antwika::input::toString(binding.key));
        return payload.dump();

        // gcov puts the cleanup block on this closing brace.
        // SaveGame.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    KeyBinding bindKeyFromPayload(const std::string &payload)
    {
        const auto parsed =
            antwika::replay::parseAndValidatePayload<OptionsFormatError>(
                payload,
                bindKeyValidator(),
                "BindingSink: game.bind_key payload");

        const auto name = parsed.at("action").get<std::string>();
        const auto action = actionFromName(name);

        if (!action.has_value())
        {
            throw OptionsFormatError(
                "antwika::game: game.bind_key names an action this "
                "build does not know: "
                + name);
        }

        // The key's own name is antwika::input's to police.
        // Its error may not be let out of a call promising this one's.
        // The handler's own no-match edge is unreachable.
        // Only an InputError can arrive, and only from above.
        // PayloadJson.hpp marks its own catch for the same reason.
        try
        {
            return KeyBinding{
                .action = *action,
                .key = antwika::input::keyFromString(
                    parsed.at("key").get<std::string>())};
        }
        catch (const antwika::input::InputError &error) // GCOVR_EXCL_LINE
        {
            throw OptionsFormatError(error.what());
        }
    }

} // namespace antwika::game
