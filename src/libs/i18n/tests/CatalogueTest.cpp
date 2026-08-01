#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <string_view>

#include <antwika/i18n/Catalogue.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/MessageId.hpp>

namespace
{

    using antwika::i18n::Catalogue;
    using antwika::i18n::CatalogueEntry;
    using antwika::i18n::catalogueFor;
    using antwika::i18n::kAllLocales;
    using antwika::i18n::kAllMessageIds;
    using antwika::i18n::kDefaultLocale;
    using antwika::i18n::kMessageCount;
    using antwika::i18n::Locale;
    using antwika::i18n::MessageId;
    using antwika::i18n::nameOf;
    using antwika::i18n::tagOf;

    std::set<MessageId> idsOf(const Catalogue &catalogue)
    {
        std::set<MessageId> ids;

        for (const CatalogueEntry &entry : catalogue.entries())
        {
            ids.insert(entry.id);
        }

        return ids;
    }

    TEST(CatalogueTest, Locale_ReportsWhatTheCatalogueWasBuiltWith)
    {
        constexpr std::array<CatalogueEntry, 1> entries{{
            {MessageId::MenuPlayGame, "spela"},
        }};

        const Catalogue catalogue{Locale::Swedish, entries};

        EXPECT_EQ(catalogue.locale(), Locale::Swedish);
        EXPECT_EQ(catalogue.entries().size(), 1U);
        EXPECT_EQ(catalogue.entries()[0].text, "spela");
    }

    TEST(CatalogueTest, Find_ReturnsTheTextForAnIdItCarries)
    {
        constexpr std::array<CatalogueEntry, 2> entries{{
            {MessageId::MenuPlayGame, "one"},
            {MessageId::MenuLanguage, "two"},
        }};

        const Catalogue catalogue{Locale::English, entries};

        EXPECT_EQ(catalogue.find(MessageId::MenuPlayGame), "one");
        EXPECT_EQ(catalogue.find(MessageId::MenuLanguage), "two");
    }

    TEST(CatalogueTest, Find_ReturnsNothingForAnIdItDoesNotCarry)
    {
        constexpr std::array<CatalogueEntry, 1> entries{{
            {MessageId::MenuPlayGame, "one"},
        }};

        const Catalogue catalogue{Locale::English, entries};

        EXPECT_EQ(
            catalogue.find(MessageId::MenuLanguage), std::nullopt);
    }

    TEST(CatalogueTest, CatalogueFor_ReturnsTheCatalogueOfEachLocale)
    {
        for (const Locale locale : kAllLocales)
        {
            EXPECT_EQ(catalogueFor(locale).locale(), locale);
        }
    }

    TEST(CatalogueTest, CatalogueFor_FallsBackToTheDefaultWhenUnknown)
    {
        EXPECT_EQ(
            &catalogueFor(static_cast<Locale>(200)),
            &catalogueFor(kDefaultLocale));
    }

    TEST(CatalogueTest, CatalogueFor_ResolvesEveryIdInEveryLocale)
    {
        for (const Locale locale : kAllLocales)
        {
            const Catalogue &catalogue = catalogueFor(locale);

            for (const MessageId id : kAllMessageIds)
            {
                const std::optional<std::string_view> text =
                    catalogue.find(id);

                ASSERT_TRUE(text.has_value())
                    << "locale " << tagOf(locale) << " is missing an id";
                EXPECT_FALSE(text->empty());
            }
        }
    }

    // This is why the catalogues are keyed by a symbolic id at all.
    // With the English text as the key it could not be written.
    // A missing Swedish entry would simply be an English one.
    TEST(CatalogueTest, CatalogueFor_CoversExactlyTheSameIdSetInEveryLocale)
    {
        const std::set<MessageId> expected{
            kAllMessageIds.begin(), kAllMessageIds.end()};

        for (const Locale locale : kAllLocales)
        {
            const Catalogue &catalogue = catalogueFor(locale);

            EXPECT_EQ(idsOf(catalogue), expected)
                << "locale " << tagOf(locale) << " covers a different set";
            EXPECT_EQ(catalogue.entries().size(), kMessageCount)
                << "locale " << tagOf(locale) << " repeats an id";
        }
    }

    TEST(CatalogueTest, CatalogueFor_KeepsTheToolbarLabelsInUseToday)
    {
        const Catalogue &english = catalogueFor(Locale::English);

        EXPECT_EQ(english.find(MessageId::ToolbarZoomIn), "zoom in");
        EXPECT_EQ(english.find(MessageId::ToolbarZoomOut), "zoom out");
        EXPECT_EQ(english.find(MessageId::ToolbarResetView), "reset view");
        EXPECT_EQ(english.find(MessageId::ToolbarZoomLevel), "zoom {0}");
    }

    // The ids whose two texts genuinely read the same in both.
    // A notation, a loanword or a noise an animal makes.
    // Written out rather than tolerated wherever it happens.
    // A forgotten Swedish entry looks exactly like one of these.
    // So the only way to be excused is to be named here.
    constexpr std::array<MessageId, 11> kSameInBothLocales{
        MessageId::ToolbarZoomLevel,
        MessageId::AtlasPixelUnknown,
        MessageId::AtlasPixelAt,
        MessageId::CompanionHunger,
        MessageId::CompanionSayLaLaLa,
        MessageId::CompanionSayZzz,
        MessageId::CompanionSayWheee,
        MessageId::CompanionDay,
        MessageId::UiDemoPageLayout,
        MessageId::TaskWorkerTick,
        MessageId::TaskWorkerBudget,
    };

    TEST(CatalogueTest, CatalogueFor_TranslatesTheMenuIntoSwedish)
    {
        const Catalogue &swedish = catalogueFor(Locale::Swedish);
        const Catalogue &english = catalogueFor(Locale::English);

        for (const MessageId id : kAllMessageIds)
        {
            const auto same = std::find(
                kSameInBothLocales.begin(), kSameInBothLocales.end(), id);

            if (same != kSameInBothLocales.end())
            {
                continue;
            }

            EXPECT_NE(swedish.find(id), english.find(id))
                << "an untranslated entry would read the same: "
                << nameOf(id);
        }
    }

} // namespace
