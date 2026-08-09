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

        return {
            "!" + std::string{nameOf<Messages>(id)} + "!",
            TranslationOrigin::Missing};
    }

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

}
