#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/i18n/MessageSet.hpp>

#include "SampleMessages.hpp"

namespace
{

    using antwika::i18n::MessageSet;
    using antwika::i18n::tests::SampleMessages;

    enum class BareId : std::uint16_t
    {
        Only,
        Count,
    };

    struct IdOnlyMessages final
    {
        using Id = BareId;
    };

    TEST(MessageSetTest, MessageSet_AcceptsATypeWithNamesAndCatalogues)
    {
        EXPECT_TRUE(MessageSet<SampleMessages>);
    }

    TEST(MessageSetTest, MessageSet_RefusesATypeThatOnlyNamesAnIdType)
    {
        EXPECT_FALSE(MessageSet<IdOnlyMessages>);
    }

}
