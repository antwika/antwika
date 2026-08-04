#include "antwika/game/KeyboardEvent.hpp"

#include <nlohmann/json-schema.hpp>
#include <nlohmann/json.hpp>

#include <antwika/replay/PayloadJson.hpp>

#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json setKeyboardSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "game.set_keyboard payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"keyboard"}; // GCOVR_EXCL_LINE
            schema["properties"]["keyboard"]["type"] = "string";
            return schema;
        }

        const nlohmann::json_schema::json_validator &
        setKeyboardValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                setKeyboardSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    std::string setKeyboardPayload(KeyboardLayout layout)
    {
        nlohmann::json payload;
        payload["keyboard"] = std::string(keyboardLayoutName(layout));
        return payload.dump();

        // gcov puts the cleanup block on this closing brace.
        // BindingEvent.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    KeyboardLayout keyboardFromPayload(const std::string &payload)
    {
        const auto parsed =
            antwika::replay::parseAndValidatePayload<OptionsFormatError>(
                payload,
                setKeyboardValidator(),
                "BindingSink: game.set_keyboard payload");

        const auto named = parsed.at("keyboard").get<std::string>();
        const auto layout = keyboardLayoutFromName(named);

        if (!layout.has_value())
        {
            throw OptionsFormatError(
                "antwika::game: game.set_keyboard names a layout this "
                "build does not know: "
                + named);
        }

        return *layout;
    }

} // namespace antwika::game
