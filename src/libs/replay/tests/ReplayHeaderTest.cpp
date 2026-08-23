#include <gtest/gtest.h>

#include <antwika/geometry/Size.hpp>

#include "antwika/replay/ReplayHeader.hpp"
#include "antwika/replay/ReplayVersions.hpp"

using antwika::geometry::Size;
using antwika::replay::kReplayDocumentVersion;
using antwika::replay::ReplayHeader;

namespace
{
    [[nodiscard]] ReplayHeader getAHeader()
    {
        return ReplayHeader{
            .version = kReplayDocumentVersion,
            .canvasSize = Size{.width = 1024, .height = 640}};
    }
}

TEST(ReplayHeaderTest, OperatorEquals_MatchesAnIdenticalHeader)
{
    EXPECT_EQ(getAHeader(), getAHeader());
}

TEST(ReplayHeaderTest, OperatorEquals_SeparatesADifferentVersion)
{
    ReplayHeader otherHeader = getAHeader();
    otherHeader.version = kReplayDocumentVersion + 1;

    EXPECT_NE(getAHeader(), otherHeader);
}

TEST(ReplayHeaderTest, OperatorEquals_SeparatesOneStatedCanvas)
{
    ReplayHeader otherHeader = getAHeader();
    otherHeader.canvasSize.reset();

    EXPECT_NE(getAHeader(), otherHeader);
}

TEST(ReplayHeaderTest, OperatorEquals_SeparatesTwoUnequalCanvases)
{
    ReplayHeader otherHeader = getAHeader();
    otherHeader.canvasSize = Size{.width = 800, .height = 600};

    EXPECT_NE(getAHeader(), otherHeader);
}

TEST(ReplayHeaderTest, OperatorEquals_MatchesTwoHeadersWithNoCanvas)
{
    EXPECT_EQ(ReplayHeader{}, ReplayHeader{});
}

TEST(ReplayHeaderTest, Ctor_DefaultsToThisBuildsVersion)
{
    const ReplayHeader header;

    EXPECT_EQ(header.version, kReplayDocumentVersion);
    EXPECT_FALSE(header.canvasSize.has_value());
}
