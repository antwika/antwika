#include <cstdint>

#include <gtest/gtest.h>

#include "antwika/i18n/CompiledMessages.hpp"
#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/MessageSet.hpp"
#include "antwika/i18n/MessageTable.hpp"

namespace antwika::i18n
{

    namespace
    {

        enum class SetId : std::uint16_t
        {
            Yes,
            No,
            Count,
        };

        constexpr MessageTable<SetId> kSetTable{
            .names{{
                {SetId::Yes, "Yes"},
                {SetId::No, "No"},
            }},
            .english{{
                {SetId::Yes, "yes"},
                {SetId::No, "no"},
            }},
            .swedish{{
                {SetId::Yes, "ja"},
                {SetId::No, "nej"},
            }},
        };

        using SetMessages = CompiledMessages<SetId, kSetTable>;

        static_assert(
            MessageSet<SetMessages>,
            "a CompiledMessages must satisfy antwika::i18n::MessageSet");

    } // namespace

    TEST(CompiledMessagesTest, Names_AreTheTablesOwnNames)
    {
        const auto names = SetMessages::names();

        ASSERT_EQ(names.size(), 2U);
        EXPECT_EQ(names[0].id, SetId::Yes);
        EXPECT_EQ(names[0].name, "Yes");
        EXPECT_EQ(names[1].name, "No");
        EXPECT_EQ(names.data(), kSetTable.names.data());
    }

    TEST(CompiledMessagesTest, CatalogueFor_ServesEachLocalesOwnEntries)
    {
        const Catalogue<SetId> &english =
            SetMessages::catalogueFor(Locale::English);
        const Catalogue<SetId> &swedish =
            SetMessages::catalogueFor(Locale::Swedish);

        EXPECT_EQ(english.locale(), Locale::English);
        EXPECT_EQ(english.find(SetId::No), "no");
        EXPECT_EQ(swedish.locale(), Locale::Swedish);
        EXPECT_EQ(swedish.find(SetId::No), "nej");
    }

    TEST(CompiledMessagesTest, CatalogueFor_ReturnsTheSameObjectEachTime)
    {
        EXPECT_EQ(
            &SetMessages::catalogueFor(Locale::Swedish),
            &SetMessages::catalogueFor(Locale::Swedish));
    }

    TEST(CompiledMessagesTest, CatalogueFor_FallsBackToTheDefault)
    {
        EXPECT_EQ(
            &SetMessages::catalogueFor(static_cast<Locale>(200)),
            &SetMessages::catalogueFor(kDefaultLocale));
    }

} // namespace antwika::i18n
