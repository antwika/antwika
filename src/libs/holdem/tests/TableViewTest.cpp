#include <gtest/gtest.h>

#include <antwika/holdem/TableView.hpp>

using antwika::holdem::TableView;

TEST(TableViewTest, TableView_DefaultsToLettingTheSeatRaise)
{
    const TableView view;

    EXPECT_TRUE(view.mayRaise);
}
