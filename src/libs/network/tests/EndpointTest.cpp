#include "antwika/network/Endpoint.hpp"

#include <gtest/gtest.h>

#include <map>
#include <string>

#include "antwika/network/Port.hpp"

using antwika::network::Endpoint;
using antwika::network::Port;

TEST(EndpointTest, EqualityComparesEveryFieldIndependently)
{
    const Endpoint endpoint{.host = "localhost", .port = Port{9000}};

    EXPECT_EQ(endpoint, (Endpoint{.host = "localhost", .port = Port{9000}}));
    EXPECT_NE(endpoint, (Endpoint{.host = "elsewhere", .port = Port{9000}}));
    EXPECT_NE(endpoint, (Endpoint{.host = "localhost", .port = Port{9001}}));
}

// The host decides first, so a lower port on a later host stays later.
TEST(EndpointTest, OrderingComparesTheHostBeforeThePort)
{
    const Endpoint early{.host = "aaa", .port = Port{9999}};
    const Endpoint late{.host = "bbb", .port = Port{1}};

    EXPECT_LT(early, late);
    EXPECT_GT(late, early);
}

TEST(EndpointTest, OrderingFallsBackToThePortOnOneHost)
{
    const Endpoint lower{.host = "localhost", .port = Port{1}};
    const Endpoint higher{.host = "localhost", .port = Port{2}};

    EXPECT_LT(lower, higher);
    EXPECT_GT(higher, lower);
}

TEST(EndpointTest, OrderingReportsTwoEqualEndpointsEquivalent)
{
    const Endpoint endpoint{.host = "localhost", .port = Port{9000}};
    const Endpoint same{.host = "localhost", .port = Port{9000}};

    EXPECT_LE(endpoint, same);
    EXPECT_GE(endpoint, same);
}

// The reason it is ordered at all -- see LoopbackBackend.
TEST(EndpointTest, AnEndpointIsAKeyAnOrderedMapCanHold)
{
    std::map<Endpoint, int> hosts;

    hosts[Endpoint{.host = "localhost", .port = Port{1}}] = 1;
    hosts[Endpoint{.host = "localhost", .port = Port{2}}] = 2;

    EXPECT_EQ(hosts.size(), 2U);
    EXPECT_EQ(hosts.at(Endpoint{.host = "localhost", .port = Port{2}}), 2);
}
