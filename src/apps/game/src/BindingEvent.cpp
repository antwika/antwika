#include "antwika/game/BindingEvent.hpp"

#include <nlohmann/json.hpp>

#include <antwika/input/InputError.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json bindKeySchema()
        {
            nlohmann::json schema = replay::documentShape(
                "game.bind_key payload", {"action", "key"});
            schema["properties"]["action"] = replay::wordShape();
            schema["properties"]["key"] = replay::wordShape();
            return schema;
        } // GCOVR_EXCL_LINE
    }

    std::string bindKeyPayload(KeyBinding binding)
    {
        nlohmann::json payload;
        payload["action"] = std::string(actionName(binding.action));
        payload["key"] =
            std::string(antwika::input::toString(binding.key));
        return payload.dump();

    } // GCOVR_EXCL_LINE

    KeyBinding bindKeyFromPayload(const std::string &payload)
    {
        const auto parsed =
            antwika::replay::parseAndValidatePayload<OptionsFormatError>(
                payload,
                replay::validatorFor<bindKeySchema>(),
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

}
