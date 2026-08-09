#pragma once

#include <concepts>
#include <span>
#include <string_view>
#include <type_traits>

#include "antwika/i18n/Catalogue.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageName.hpp"

namespace antwika::i18n
{

    template <typename Messages>
    concept MessageSet =
        std::is_enum_v<typename Messages::Id>
        && requires(Locale locale) {
               {
                   Messages::names()
               } -> std::same_as<
                     std::span<const MessageName<typename Messages::Id>>>;
               {
                   Messages::catalogueFor(locale)
               } -> std::same_as<const Catalogue<typename Messages::Id> &>;
           };

    template <MessageSet Messages>
    [[nodiscard]] std::string_view nameOf(
        typename Messages::Id id) noexcept
    {
        using Id = typename Messages::Id;

        for (const MessageName<Id> &named : Messages::names())
        {
            if (named.id == id)
            {
                return named.name;
            }
        }

        return "?";
    }

}
