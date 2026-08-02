#pragma once

#include <array>
#include <cstdint>
#include <span>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageName.hpp>

namespace antwika::i18n::tests
{

    /**
     * @brief A message set that exists only to exercise the machinery.
     *
     * The library's own ids are the two language names, and both are in
     * both catalogues, so nothing declared there can reach the fallback
     * rule or the miss.
     * This set is deliberately incomplete instead: Swedish is silent
     * about Language, and Absent is in no catalogue at all, which is
     * what lets a test watch each of the three origins happen.
     */
    enum class TestId : std::uint16_t
    {
        Play,
        Language,
        Level,
        Absent,
        Count,
    };

    inline constexpr std::array<MessageName<TestId>, 4> kTestNames{{
        {TestId::Play, "Play"},
        {TestId::Language, "Language"},
        {TestId::Level, "Level"},
        {TestId::Absent, "Absent"},
    }};

    inline constexpr std::array<CatalogueEntry<TestId>, 3> kTestEnglish{{
        {TestId::Play, "Play game"},
        {TestId::Language, "Language"},
        {TestId::Level, "zoom {0} of {1}"},
    }};

    inline constexpr std::array<CatalogueEntry<TestId>, 2> kTestSwedish{{
        {TestId::Play, "Spela"},
        {TestId::Level, "zoom {0} av {1}"},
    }};

    inline constexpr Catalogue<TestId> kTestEnglishCatalogue{
        Locale::English, kTestEnglish};

    inline constexpr Catalogue<TestId> kTestSwedishCatalogue{
        Locale::Swedish, kTestSwedish};

    /**
     * @brief The incomplete set above, as a MessageSet.
     */
    struct TestMessages final
    {
        using Id = TestId;

        [[nodiscard]] static std::span<const MessageName<TestId>>
            names() noexcept
        {
            return kTestNames;
        }

        [[nodiscard]] static const Catalogue<TestId> &catalogueFor(
            Locale locale) noexcept
        {
            switch (locale)
            {
            case Locale::English:
                return kTestEnglishCatalogue;
            case Locale::Swedish:
                return kTestSwedishCatalogue;
            }

            return catalogueFor(kDefaultLocale);
        }
    };

} // namespace antwika::i18n::tests
