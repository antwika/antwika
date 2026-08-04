#pragma once

#include <string>

#include <antwika/i18n/Locale.hpp>

namespace antwika::game
{

    using antwika::i18n::Locale;

    /**
     * @brief Encode a starting language as a game.set_locale payload.
     *
     * Written as the locale's language tag rather than as the
     * enumerator's number, for the reason bindKeyPayload writes key
     * names: a recording is read by builds this one has never met, and
     * an enumerator's position is not a promise either of them made.
     *
     * @param locale The language to announce.
     * @return The payload, a JSON object of one string.
     */
    [[nodiscard]] std::string setLocalePayload(Locale locale);

    /**
     * @brief Decode a game.set_locale payload.
     * @param payload The event's raw payload string.
     * @return The language it names.
     * @throws OptionsFormatError If the payload is not valid JSON, is
     * not this shape, or names a tag this build has no catalogue for.
     */
    [[nodiscard]] Locale localeFromPayload(const std::string &payload);

} // namespace antwika::game
