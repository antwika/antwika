#pragma once

#include <array>
#include <cstdint>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

namespace antwika::i18n::tests
{

    enum class SampleId : std::uint16_t
    {
        Play,
        Language,
        Level,
        Absent,
        Count,
    };

    inline constexpr std::array<MessageName<SampleId>, 4> kSampleNames{{
        {SampleId::Play, "Play"},
        {SampleId::Language, "Language"},
        {SampleId::Level, "Level"},
        {SampleId::Absent, "Absent"},
    }};

    inline constexpr std::array<CatalogueEntry<SampleId>, 3> kSampleEnglish{{
        {SampleId::Play, "Play game"},
        {SampleId::Language, "Language"},
        {SampleId::Level, "zoom {0} of {1}"},
    }};

    inline constexpr std::array<CatalogueEntry<SampleId>, 2> kSampleSwedish{{
        {SampleId::Play, "Spela"},
        {SampleId::Level, "zoom {0} av {1}"},
    }};

    inline constexpr Catalogue<SampleId> kSampleEnglishCatalogue{
        Locale::English, kSampleEnglish};

    inline constexpr Catalogue<SampleId> kSampleSwedishCatalogue{
        Locale::Swedish, kSampleSwedish};

    struct SampleMessages final
    {
        using Id = SampleId;

        [[nodiscard]] static std::span<const MessageName<SampleId>>
            names() noexcept
        {
            return kSampleNames;
        }

        [[nodiscard]] static const Catalogue<SampleId> &catalogueFor(
            Locale locale) noexcept
        {
        return antwika::i18n::pickCatalogue(
            locale, kSampleEnglishCatalogue, kSampleSwedishCatalogue);
        }
    };

}
