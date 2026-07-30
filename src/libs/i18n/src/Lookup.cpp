#include "antwika/i18n/Lookup.hpp"

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/Substitute.hpp"
#include "antwika/i18n/Translation.hpp"

namespace antwika::i18n
{

    namespace
    {

        // Exclamation marks rather than an empty string.
        // A gap should be noticed in a screenshot, not merely look short.
        std::string missingText(MessageId id)
        {
            return "!" + std::string{nameOf(id)} + "!";
        }

    } // namespace

    Translation lookup(
        MessageId id, const Catalogue &active, const Catalogue &fallback)
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

        return {missingText(id), TranslationOrigin::Missing};
    }

    Translation format(
        MessageId id,
        std::span<const std::string_view> args,
        const Catalogue &active,
        const Catalogue &fallback)
    {
        Translation resolved = lookup(id, active, fallback);
        resolved.text = substitute(resolved.text, args);

        return resolved;
    } // GCOVR_EXCL_LINE

} // namespace antwika::i18n
