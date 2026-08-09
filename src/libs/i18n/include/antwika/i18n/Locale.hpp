#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/i18n/MessageId.hpp"

namespace antwika::i18n
{

    enum class Locale : std::uint8_t
    {
        English,

        Swedish,
    };

    inline constexpr Locale kDefaultLocale{Locale::English};

    inline constexpr std::array<Locale, 2> kAllLocales{
        Locale::English,
        Locale::Swedish,
    };

    [[nodiscard]] std::string_view tagOf(Locale locale) noexcept;

    [[nodiscard]] std::optional<Locale> localeFromTag(
        std::string_view tag) noexcept;

    [[nodiscard]] MessageId nameIdOf(Locale locale) noexcept;

}
