#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/MessageSet.hpp"
#include "antwika/i18n/Substitute.hpp"
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
     * The catalogues are parameters rather than something looked up from
     * Messages, so a test can hand this a deliberately incomplete one and
     * watch the fallback rule work.
     *
     * @tparam Messages The message set the id belongs to.
     * @param id The id to resolve.
     * @param active The catalogue for the locale in use.
     * @param fallback The catalogue consulted when the active one is
     *        silent, normally Messages::catalogueFor(kDefaultLocale).
     * @return The text and where it came from.
     */
    template <MessageSet Messages>
    [[nodiscard]] Translation lookup(
        typename Messages::Id id,
        const Catalogue<typename Messages::Id> &active,
        const Catalogue<typename Messages::Id> &fallback)
    {
        if (const std::optional<std::string_view> text = active.find(id);
            text.has_value())
        {
            return {std::string{*text}, TranslationOrigin::Exact};
        }

        if (const std::optional<std::string_view> text = fallback.find(id);
            text.has_value())
        {
            return {std::string{*text}, TranslationOrigin::Fallback};
        }

        // Exclamation marks rather than an empty string.
        // A gap should be noticed in a screenshot, not merely look short.
        return {
            "!" + std::string{nameOf<Messages>(id)} + "!",
            TranslationOrigin::Missing};
    }

    /**
     * @brief Resolve one id and substitute positional arguments into it.
     *
     * Substitution happens whatever the origin, so a fallback and a miss
     * are formatted the same way an exact hit is.
     *
     * @tparam Messages The message set the id belongs to.
     * @param id The id to resolve.
     * @param args The arguments for the message's `{0}`-style
     *        placeholders.
     * @param active The catalogue for the locale in use.
     * @param fallback The catalogue consulted when the active one is
     *        silent.
     * @return The substituted text and where the pattern came from.
     */
    template <MessageSet Messages>
    [[nodiscard]] Translation format(
        typename Messages::Id id,
        std::span<const std::string_view> args,
        const Catalogue<typename Messages::Id> &active,
        const Catalogue<typename Messages::Id> &fallback)
    {
        Translation resolved = lookup<Messages>(id, active, fallback);
        resolved.text = substitute(resolved.text, args);

        return resolved;
    } // GCOVR_EXCL_LINE

} // namespace antwika::i18n
