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
}

TEST(ReplayHeaderTest, OperatorEquals_MatchesAnIdenticalHeader)
{
    EXPECT_EQ(aHeader(), aHeader());
}

TEST(ReplayHeaderTest, OperatorEquals_SeparatesADifferentVersion)
{
    ReplayHeader other = aHeader();
    other.version = kReplayDocumentVersion + 1;

    EXPECT_NE(aHeader(), other);
}

TEST(ReplayHeaderTest, OperatorEquals_SeparatesOneStatedCanvas)
{
    ReplayHeader other = aHeader();
    other.canvas.reset();

    EXPECT_NE(aHeader(), other);
}

TEST(ReplayHeaderTest, OperatorEquals_SeparatesTwoUnequalCanvases)
{
    ReplayHeader other = aHeader();
    other.canvas = Size{.width = 800, .height = 600};

    EXPECT_NE(aHeader(), other);
}

TEST(ReplayHeaderTest, OperatorEquals_MatchesTwoHeadersWithNoCanvas)
{
    EXPECT_EQ(ReplayHeader{}, ReplayHeader{});
}

TEST(ReplayHeaderTest, Ctor_DefaultsToThisBuildsVersion)
{
    const ReplayHeader header;

    EXPECT_EQ(header.version, kReplayDocumentVersion);
    EXPECT_FALSE(header.canvas.has_value());
}
