#include <gtest/gtest.h>

#include <set>
#include <string_view>

#include "antwika/game/Action.hpp"
#include "antwika/game/MessageId.hpp"

using antwika::game::Action;
using antwika::game::actionFromName;
using antwika::game::actionIndex;
using antwika::game::actionLabel;
using antwika::game::actionName;
using antwika::game::kActionCount;
using antwika::game::kActions;

// A name is what a file and a recording hold.
// So every action must have one and no two may share one.
TEST(ActionTest, EveryActionHasANameOfItsOwn)
{
    std::set<std::string_view> names;

    for (const auto action : kActions)
    {
        names.insert(actionName(action));
    }

    EXPECT_EQ(names.size(), kActionCount);
}

// The round trip a persisted binding is read back through.
TEST(ActionTest, ANameRoundTripsBackToItsAction)
{
    for (const auto action : kActions)
    {
        EXPECT_EQ(actionFromName(actionName(action)), action);
    }
}

// A name no action goes by is answered rather than thrown about.
TEST(ActionTest, ANameNoActionGoesByIsNothing)
{
    EXPECT_FALSE(actionFromName("fly").has_value());
    EXPECT_FALSE(actionFromName("").has_value());
}

// Every action is listed exactly once, in its own index's place.
TEST(ActionTest, EveryActionIsListedAtItsOwnIndex)
{
    for (std::size_t index = 0; index < kActions.size(); ++index)
    {
        EXPECT_EQ(actionIndex(kActions[index]), index);
    }
}

// A caption is an id rather than a word, and each action has its own.
TEST(ActionTest, EveryActionHasACaptionOfItsOwn)
{
    std::set<antwika::game::MessageId> labels;

    for (const auto action : kActions)
    {
        labels.insert(actionLabel(action));
    }

    EXPECT_EQ(labels.size(), kActionCount);
}
