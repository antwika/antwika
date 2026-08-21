#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "antwika/io/FileNames.hpp"

using antwika::io::filteredBySuffix;
using antwika::io::withSuffix;

TEST(FileNamesTest, FilteredBySuffix_TakesOnlyWhatEndsInTheSuffix)
{
    const std::vector<std::string> names{
        "keep.json", "atlas-15x9.png", "notes", "map.json.bak"};

    EXPECT_EQ(
        filteredBySuffix(names, ".json", 24),
        (std::vector<std::string>{"keep.json"}));
}

TEST(FileNamesTest, FilteredBySuffix_ListsThemInTheSameOrderEveryTime)
{
    const std::vector<std::string> names{
        "second.json", "first.json", "third.json"};

    EXPECT_EQ(
        filteredBySuffix(names, ".json", 24),
        (std::vector<std::string>{
            "first.json", "second.json", "third.json"}));
}

TEST(FileNamesTest, FilteredBySuffix_ListsNoMoreThanThereIsRoomFor)
{
    std::vector<std::string> names;

    for (std::size_t index = 0; index < 12; ++index)
    {
        names.push_back(
            "map" + std::to_string(100 + index) + ".json");
    }

    EXPECT_EQ(filteredBySuffix(names, ".json", 8).size(), 8U);
}

TEST(FileNamesTest, FilteredBySuffix_TakesNothingFromAnEmptyFolder)
{
    EXPECT_TRUE(
        filteredBySuffix(std::vector<std::string>{}, ".json", 24)
            .empty());
}

TEST(FileNamesTest, FilteredBySuffix_AsksForMoreThanOnlyTheSuffix)
{
    const std::vector<std::string> names{".json", "a.json"};

    EXPECT_EQ(
        filteredBySuffix(names, ".json", 24),
        (std::vector<std::string>{"a.json"}));
}

TEST(FileNamesTest, WithSuffix_PutsTheSuffixOnAName)
{
    EXPECT_EQ(withSuffix("keep", ".json"), "keep.json");
}

TEST(FileNamesTest, WithSuffix_LeavesANameThatHasItAlone)
{
    EXPECT_EQ(withSuffix("keep.json", ".json"), "keep.json");
    EXPECT_EQ(withSuffix(".json", ".json"), ".json.json");
}
