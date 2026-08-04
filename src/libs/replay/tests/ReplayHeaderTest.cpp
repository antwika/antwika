#include <gtest/gtest.h>

#include <antwika/geometry/Size.hpp>

#include "antwika/replay/ReplayHeader.hpp"
#include "antwika/replay/SchemaVersion.hpp"

using antwika::geometry::Size;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::ReplayHeader;

namespace
{
    [[nodiscard]] ReplayHeader aHeader()
    {
        return ReplayHeader{
            .version = kReplayDocumentVersion,
            .canvas = Size{.width = 1024, .height = 640}};
    }
} // namespace

TEST(ReplayHeaderTest, EqualsAnIdenticalHeader)
{
    EXPECT_EQ(aHeader(), aHeader());
}

TEST(ReplayHeaderTest, DiffersWhenTheVersionDiffers)
{
    ReplayHeader other = aHeader();
    other.version = kReplayDocumentVersion + 1;

    EXPECT_NE(aHeader(), other);
}

TEST(ReplayHeaderTest, DiffersWhenOnlyOneSideStatedItsCanvas)
{
    ReplayHeader other = aHeader();
    other.canvas.reset();

    EXPECT_NE(aHeader(), other);
}

TEST(ReplayHeaderTest, DiffersWhenBothCanvasesAreSetButNotEqual)
{
    ReplayHeader other = aHeader();
    other.canvas = Size{.width = 800, .height = 600};

    EXPECT_NE(aHeader(), other);
}

TEST(ReplayHeaderTest, TwoHeadersThatStateNoCanvasAreEqual)
{
    EXPECT_EQ(ReplayHeader{}, ReplayHeader{});
}

// The version a writer states without being asked.
TEST(ReplayHeaderTest, DefaultsToTheVersionThisBuildWrites)
{
    const ReplayHeader header;

    EXPECT_EQ(header.version, kReplayDocumentVersion);
    EXPECT_FALSE(header.canvas.has_value());
}
