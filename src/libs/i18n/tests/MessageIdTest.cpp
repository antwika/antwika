#include <gtest/gtest.h>

#include <set>
#include <string_view>

#include <antwika/i18n/MessageId.hpp>

namespace
{

    using antwika::i18n::kAllMessageIds;
    using antwika::i18n::kMessageCount;
    using antwika::i18n::MessageId;
    using antwika::i18n::nameOf;

    TEST(MessageIdTest, AllMessageIds_ListsEveryIdExactlyOnce)
    {
        const std::set<MessageId> unique{
            kAllMessageIds.begin(), kAllMessageIds.end()};

        EXPECT_EQ(kAllMessageIds.size(), kMessageCount);
        EXPECT_EQ(unique.size(), kMessageCount);
    }

    TEST(MessageIdTest, NameOf_GivesEveryIdItsOwnDistinctName)
    {
        std::set<std::string_view> names;

        for (const MessageId id : kAllMessageIds)
        {
            const std::string_view name = nameOf(id);

            EXPECT_FALSE(name.empty());
            EXPECT_NE(name, "?");

            names.insert(name);
        }

        EXPECT_EQ(names.size(), kMessageCount);
    }

    TEST(MessageIdTest, NameOf_ReportsQuestionMarkForUnknownId)
    {
        EXPECT_EQ(nameOf(static_cast<MessageId>(9999)), "?");
    }

} // namespace
