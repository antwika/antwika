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

TEST(ActionTest, ActionName_EveryActionHasANameOfItsOwn)
{
    std::set<std::string_view> names;

    for (const auto action : kActions)
    {
        names.insert(actionName(action));
    }

    EXPECT_EQ(names.size(), kActionCount);
}

TEST(ActionTest, ActionFromName_ANameRoundTripsBackToItsAction)
{
    for (const auto action : kActions)
    {
        EXPECT_EQ(actionFromName(actionName(action)), action);
    }
}

TEST(ActionTest, ActionFromName_ANameNoActionGoesByIsNothing)
{
    EXPECT_FALSE(actionFromName("fly").has_value());
    EXPECT_FALSE(actionFromName("").has_value());
}

TEST(ActionTest, ActionIndex_EveryActionIsListedAtItsOwnIndex)
{
    for (std::size_t index = 0; index < kActions.size(); ++index)
    {
        EXPECT_EQ(actionIndex(kActions[index]), index);
    }
}

TEST(ActionTest, ActionLabel_EveryActionHasACaptionOfItsOwn)
{
    std::set<antwika::game::MessageId> labels;

    for (const auto action : kActions)
    {
        labels.insert(actionLabel(action));
    }

    EXPECT_EQ(labels.size(), kActionCount);
}
