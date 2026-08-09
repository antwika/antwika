#include <gtest/gtest.h>

#include <set>
#include <string_view>

#include "SaveSections.hpp"

using antwika::game::saveSections;

TEST(SaveSectionsTest, SaveSections_NameEverySectionOnce)
{
    std::set<std::string_view> named;

    for (const auto &section : saveSections())
    {
        EXPECT_FALSE(section.name.empty());
        EXPECT_TRUE(named.insert(section.name).second) << section.name;
    }

    EXPECT_EQ(named.size(), saveSections().size());
}

TEST(SaveSectionsTest, SaveSections_ReadBackEverySectionTheyWrite)
{
    for (const auto &section : saveSections())
    {
        EXPECT_EQ(section.encode != nullptr, section.decode != nullptr)
            << section.name;
    }
}

TEST(SaveSectionsTest, SaveSections_GiveEverySectionSomethingToDo)
{
    for (const auto &section : saveSections())
    {
        EXPECT_TRUE(
            section.describe != nullptr || section.encode != nullptr)
            << section.name;
    }
}
