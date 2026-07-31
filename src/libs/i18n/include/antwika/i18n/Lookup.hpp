#pragma once

#include <span>
#include <string_view>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/Translation.hpp"

namespace antwika::i18n
{

    /**
     * @brief Resolve one id against an active catalogue and a fallback.
     *
     * Total and deterministic: the active catalogue is asked first, then
     * the fallback, and an id neither knows resolves to its own name in
     * exclamation marks (`"!ToolbarZoomIn!"`).
     * Throwing on a miss was rejected because a lookup runs while a frame
     * is being drawn, and a missing translation would then take the
     * program down in front of the user rather than showing a slightly
     * wrong label.
     * The miss is not swallowed either -- Translation::origin says which
     * of the three happened, so a test or a diagnostic can insist on
     * TranslationOrigin::Exact where prose alone could not tell a
     * fallback from a translation that reads the same in both languages.
     *
     * @param id The id to resolve.
     * @param active The catalogue for the locale in use.
     * @param fallback The catalogue consulted when the active one is
     *        silent, normally catalogueFor(kDefaultLocale).
     * @return The text and where it came from.
     */
    [[nodiscard]] Translation lookup(
        MessageId id, const Catalogue &active, const Catalogue &fallback);

    /**
     * @brief Resolve one id and substitute positional arguments into it.
     *
     * Substitution happens whatever the origin, so a fallback and a miss
     * are formatted the same way an exact hit is.
     *
     * @param id The id to resolve.
     * @param args The arguments for the message's `{0}`-style
     *        placeholders.
     * @param active The catalogue for the locale in use.
     * @param fallback The catalogue consulted when the active one is
     *        silent.
     * @return The substituted text and where the pattern came from.
     */
    [[nodiscard]] Translation format(
        MessageId id,
        std::span<const std::string_view> args,
        const Catalogue &active,
        const Catalogue &fallback);

} // namespace antwika::i18n
