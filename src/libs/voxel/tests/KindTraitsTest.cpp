#include <gtest/gtest.h>

#include <optional>

#include <antwika/voxel/KindTraits.hpp>
#include <antwika/voxel/VoxelMaterial.hpp>

using antwika::voxel::isClimbable;
using antwika::voxel::isRamped;
using antwika::voxel::isSolid;
using antwika::voxel::isSwimmable;
using antwika::voxel::Kind;

TEST(KindTraitsTest, IsSolid_HoldsForEverythingButWaterAndLadders)
{
    EXPECT_TRUE(isSolid(Kind::Normal));
    EXPECT_TRUE(isSolid(Kind::Ramp));
    EXPECT_FALSE(isSolid(Kind::Water));
    EXPECT_FALSE(isSolid(Kind::Ladder));
}

TEST(KindTraitsTest, IsSwimmable_HoldsForWaterAlone)
{
    EXPECT_TRUE(isSwimmable(Kind::Water));
    EXPECT_FALSE(isSwimmable(Kind::Normal));
    EXPECT_TRUE(isClimbable(Kind::Ladder));
    EXPECT_FALSE(isClimbable(Kind::Ramp));
    EXPECT_TRUE(isRamped(Kind::Ramp));
    EXPECT_FALSE(isRamped(Kind::Normal));
}

TEST(KindTraitsTest, IsRamped_LeavesAPlainKindWithoutATrait)
{
    EXPECT_FALSE(isSwimmable(Kind::Normal));
    EXPECT_FALSE(isClimbable(Kind::Normal));
    EXPECT_FALSE(isRamped(Kind::Normal));
    EXPECT_TRUE(isSolid(Kind::Normal));
}

TEST(KindTraitsTest, IsSolid_ReadsFalseWhereThereIsNoKind)
{
    const std::optional<Kind> missingKind;

    EXPECT_FALSE(isSolid(missingKind));
    EXPECT_FALSE(isSwimmable(missingKind));
    EXPECT_FALSE(isClimbable(missingKind));
    EXPECT_FALSE(isRamped(missingKind));
}

TEST(KindTraitsTest, IsSolid_ReadsAHeldKindAsThatKind)
{
    for (const auto kind :
         {Kind::Normal, Kind::Water, Kind::Ramp, Kind::Ladder})
    {
        const std::optional<Kind> heldKind{kind};

        EXPECT_EQ(isSolid(heldKind), isSolid(kind));
        EXPECT_EQ(isSwimmable(heldKind), isSwimmable(kind));
        EXPECT_EQ(isClimbable(heldKind), isClimbable(kind));
        EXPECT_EQ(isRamped(heldKind), isRamped(kind));
    }
}
