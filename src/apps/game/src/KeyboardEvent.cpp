#include "antwika/game/KeyboardEvent.hpp"

#include <nlohmann/json.hpp>

#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json setKeyboardSchema()
        {
            nlohmann::json schema = replay::documentShape(
                "game.set_keyboard payload", {"keyboard"});
            schema["properties"]["keyboard"] = replay::wordShape();
            return schema;
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
                replay::validatorFor<setKeyboardSchema>(),
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
