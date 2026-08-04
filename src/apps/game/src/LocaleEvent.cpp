#include "antwika/game/LocaleEvent.hpp"

#include <nlohmann/json.hpp>

#include <antwika/i18n/Locale.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/game/OptionsFormatError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json setLocaleSchema()
        {
            nlohmann::json schema = replay::documentShape(
                "game.set_locale payload", {"locale"});
            schema["properties"]["locale"] = replay::wordShape();
            return schema;
        } // GCOVR_EXCL_LINE
    } // namespace

    std::string setLocalePayload(Locale locale)
    {
        nlohmann::json payload;
        payload["locale"] =
            std::string(antwika::i18n::tagOf(locale));
        return payload.dump();

        // gcov puts the cleanup block on this closing brace.
        // BindingEvent.cpp's own encoder explains it at length.
        // No input reaches it.
    } // GCOVR_EXCL_LINE

    Locale localeFromPayload(const std::string &payload)
    {
        const auto parsed =
            antwika::replay::parseAndValidatePayload<OptionsFormatError>(
                payload,
                replay::validatorFor<setLocaleSchema>(),
                "LocaleState: game.set_locale payload");

        const auto tag = parsed.at("locale").get<std::string>();
        const auto locale = antwika::i18n::localeFromTag(tag);

        if (!locale.has_value())
        {
            throw OptionsFormatError(
                "antwika::game: game.set_locale names a language this "
                "build has no catalogue for: "
                + tag);
        }

        return *locale;
    }

} // namespace antwika::game
