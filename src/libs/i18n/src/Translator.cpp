#include "antwika/i18n/Translator.hpp"

#include <span>
#include <string>
#include <string_view>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/Lookup.hpp"
#include "antwika/i18n/MessageId.hpp"
#include "antwika/i18n/Translation.hpp"

namespace antwika::i18n
{

    Translation Translator::lookup(MessageId id) const
    {
        return i18n::lookup(
            id, catalogueFor(activeLocale), catalogueFor(kDefaultLocale));
    }

    std::string Translator::text(MessageId id) const
    {
        return lookup(id).text;
    }

    Translation Translator::format(
        MessageId id, std::span<const std::string_view> args) const
    {
        return i18n::format(
            id,
            args,
            catalogueFor(activeLocale),
            catalogueFor(kDefaultLocale));
    }

    std::string Translator::formatted(
        MessageId id, std::span<const std::string_view> args) const
    {
        return format(id, args).text;
    }

} // namespace antwika::i18n
