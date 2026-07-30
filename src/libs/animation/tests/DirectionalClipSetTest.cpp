#include <gtest/gtest.h>

#include <array>

#include <antwika/animation/Clip.hpp>
#include <antwika/animation/DirectionalClipSet.hpp>
#include <antwika/animation/Facing.hpp>

namespace antwika::animation
{

    namespace
    {

        DirectionalClipSet fourWayClips()
        {
            return DirectionalClipSet(std::array<Clip, kFacingCount>{
                uniformClip(0, 2, 1),
                uniformClip(2, 2, 1),
                uniformClip(4, 2, 1),
                uniformClip(6, 2, 1),
            });
        }

    } // namespace

    TEST(DirectionalClipSetTest, ClipFor_ReturnsTheClipForEachFacing)
    {
        const DirectionalClipSet clips = fourWayClips();

        EXPECT_EQ(clips.clipFor(Facing::North).frames()[0].index, 0U);
        EXPECT_EQ(clips.clipFor(Facing::East).frames()[0].index, 2U);
        EXPECT_EQ(clips.clipFor(Facing::South).frames()[0].index, 4U);
        EXPECT_EQ(clips.clipFor(Facing::West).frames()[0].index, 6U);
    }

    TEST(DirectionalClipSetTest, ClipFor_KeepsEachClipsOwnDefinition)
    {
        const DirectionalClipSet clips = fourWayClips();

        EXPECT_EQ(clips.clipFor(Facing::South).frames().size(), 2U);
        EXPECT_EQ(clips.clipFor(Facing::South).durationTicks(), 2U);
    }

} // namespace antwika::animation
