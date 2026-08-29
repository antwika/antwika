#include <gtest/gtest.h>

#include <antwika/editor/editor/state/WorldEdit.hpp>
#include <antwika/solver/VoxelWeave.hpp>

using antwika::editor::WorldEdit;
using antwika::solver::CornerSeams;

TEST(WorldEditTest, GetEditLevel_StartsAtZero)
{
    const WorldEdit edit;

    EXPECT_EQ(edit.getEditLevel(), 0);
}

TEST(WorldEditTest, SetEditLevel_KeepsTheLevelItWasHanded)
{
    WorldEdit edit;

    edit.setEditLevel(-7);

    EXPECT_EQ(edit.getEditLevel(), -7);
}

TEST(WorldEditTest, StepLevelUp_RaisesTheLevelByOne)
{
    WorldEdit edit;

    edit.stepLevelUp();

    EXPECT_EQ(edit.getEditLevel(), 1);
}

TEST(WorldEditTest, StepLevelDown_LowersTheLevelPastZero)
{
    WorldEdit edit;

    edit.stepLevelDown();
    edit.stepLevelDown();

    EXPECT_EQ(edit.getEditLevel(), -2);
}

TEST(WorldEditTest, GetCornerJoining_StartsIgnored)
{
    const WorldEdit edit;

    EXPECT_EQ(edit.getCornerJoining(), CornerSeams::Ignored);
    EXPECT_FALSE(edit.isCornerJoiningOn());
}

TEST(WorldEditTest, ToggleCornerJoining_SwitchesTheSeamsOnAndOffAgain)
{
    WorldEdit edit;

    edit.toggleCornerJoining();

    EXPECT_EQ(edit.getCornerJoining(), CornerSeams::Included);
    EXPECT_TRUE(edit.isCornerJoiningOn());

    edit.toggleCornerJoining();

    EXPECT_EQ(edit.getCornerJoining(), CornerSeams::Ignored);
    EXPECT_FALSE(edit.isCornerJoiningOn());
}

TEST(WorldEditTest, SetCornerJoining_MapsTheSavedSettingOntoTheSeams)
{
    WorldEdit edit;

    edit.setCornerJoining(true);

    EXPECT_EQ(edit.getCornerJoining(), CornerSeams::Included);

    edit.setCornerJoining(false);

    EXPECT_EQ(edit.getCornerJoining(), CornerSeams::Ignored);
}

TEST(WorldEditTest, GetRiseAxis_RestsAtZeroWithNoKeyHeld)
{
    const WorldEdit edit;

    EXPECT_EQ(edit.getRiseAxis(), 0.0F);
    EXPECT_FALSE(edit.isRiseHeld());
}

TEST(WorldEditTest, GetRiseAxis_ClimbsWhileAscendIsHeld)
{
    WorldEdit edit;

    edit.setAscendHeld(true);

    EXPECT_EQ(edit.getRiseAxis(), 1.0F);
    EXPECT_TRUE(edit.isRiseHeld());
}

TEST(WorldEditTest, GetRiseAxis_SinksWhileDescendIsHeld)
{
    WorldEdit edit;

    edit.setDescendHeld(true);

    EXPECT_EQ(edit.getRiseAxis(), -1.0F);
    EXPECT_TRUE(edit.isRiseHeld());
}

TEST(WorldEditTest, GetRiseAxis_CancelsOutWithBothKeysHeld)
{
    WorldEdit edit;

    edit.setAscendHeld(true);
    edit.setDescendHeld(true);

    EXPECT_EQ(edit.getRiseAxis(), 0.0F);
    EXPECT_TRUE(edit.isRiseHeld());
}

TEST(WorldEditTest, SetAscendHeld_ReleasesTheKeyItHeld)
{
    WorldEdit edit;

    edit.setAscendHeld(true);
    edit.setAscendHeld(false);
    edit.setDescendHeld(true);
    edit.setDescendHeld(false);

    EXPECT_FALSE(edit.isRiseHeld());
}
