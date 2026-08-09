#pragma once

#include <span>
#include <string>
#include <string_view>

#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/Lookup.hpp"
#include "antwika/i18n/MessageSet.hpp"
#include "antwika/i18n/Translation.hpp"

namespace antwika::i18n
{

    template <MessageSet Messages>
    class Translator final
    {
    public:
        using Id = typename Messages::Id;

        explicit constexpr Translator(Locale locale) noexcept
            : activeLocale{locale}
        {
        }

        [[nodiscard]] constexpr Locale locale() const noexcept
        {
            return activeLocale;
        }

        constexpr void setLocale(Locale locale) noexcept
        {
            activeLocale = locale;
        }

        [[nodiscard]] Translation lookup(Id id) const
        {
            return i18n::lookup<Messages>(
                id,
                Messages::catalogueFor(activeLocale),
                Messages::catalogueFor(kDefaultLocale));
        }

        [[nodiscard]] std::string text(Id id) const
        {
            return lookup(id).text;
        }

        [[nodiscard]] Translation format(
            Id id, std::span<const std::string_view> args) const
        {
            return i18n::format<Messages>(
                id,
                args,
                Messages::catalogueFor(activeLocale),
                Messages::catalogueFor(kDefaultLocale));
        }

        [[nodiscard]] std::string formatted(
            Id id, std::span<const std::string_view> args) const
        {
            return format(id, args).text;
        }

    private:
        Locale activeLocale;
    };

}
