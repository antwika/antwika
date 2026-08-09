#include <gtest/gtest.h>

#include <antwika/gfx/Size.hpp>

#include "antwika/atlas_editor/EditorTheme.hpp"

using antwika::atlas_editor::filesShownIn;
using antwika::atlas_editor::labelsAbove;
using antwika::atlas_editor::Modal;
using antwika::atlas_editor::kCardLabels;
using antwika::gfx::Size;

namespace
{
    constexpr Size kCramped{.width = 200, .height = 40};

    constexpr Size kWindow{.width = 1280, .height = 720};
}

TEST(EditorThemeTest, FilesShownIn_ListsOneEntryWhereNoneWouldFit)
{
    EXPECT_EQ(filesShownIn(kCramped, kCardLabels), 1U);
}

TEST(EditorThemeTest, FilesShownIn_ListsSeveralEntriesInAWindow)
{
    EXPECT_GT(filesShownIn(kWindow, kCardLabels), 1U);
}

TEST(EditorThemeTest, LabelsAbove_SpendsMoreLinesOnTheSaveCard)
{
    EXPECT_EQ(labelsAbove(Modal::Load), kCardLabels);
    EXPECT_GT(labelsAbove(Modal::Save), labelsAbove(Modal::Load));
}

TEST(EditorThemeTest, FilesShownIn_ListsFewerEntriesUnderMoreLabels)
{
    EXPECT_LT(
        filesShownIn(kWindow, labelsAbove(Modal::Save)),
        filesShownIn(kWindow, labelsAbove(Modal::Load)));
}
