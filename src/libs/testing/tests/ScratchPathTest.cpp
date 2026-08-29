#include <gtest/gtest.h>

#include <string>

#include <antwika/testing/ScratchPath.hpp>

using antwika::testing::getScratchName;
using antwika::testing::getScratchPath;

TEST(ScratchPathTest, GetScratchPath_CarriesTheSuiteAndTheTestName)
{
    const auto leaf = getScratchPath("scratch-path").filename().string();

    EXPECT_NE(leaf.find("scratch-path"), std::string::npos);
    EXPECT_NE(leaf.find("ScratchPathTest"), std::string::npos);
    EXPECT_NE(
        leaf.find("GetScratchPath_CarriesTheSuiteAndTheTestName"),
        std::string::npos);
}

TEST(ScratchPathTest, GetScratchName_DiffersBetweenSuitesSharingATestName)
{
    const auto firstName = getScratchName("scratch", "OneTest", "SameName");
    const auto otherName = getScratchName("scratch", "TwoTest", "SameName");

    EXPECT_NE(firstName, otherName);
}

TEST(ScratchPathTest, GetScratchName_FlattensPathSeparators)
{
    const auto name = getScratchName("scratch", "Sliced/Test", "Case/0");

    EXPECT_EQ(name.find('/'), std::string::npos);
}
