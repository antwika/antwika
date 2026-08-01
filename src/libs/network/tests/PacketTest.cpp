#include "antwika/network/Packet.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "antwika/network/PeerId.hpp"

using antwika::network::Packet;
using antwika::network::PeerId;

namespace
{
    std::vector<std::byte> bytesOf(unsigned char value)
    {
        return {std::byte{value}};
    }
} // namespace

TEST(PacketTest, EqualityComparesEveryFieldIndependently)
{
    const Packet packet{.from = PeerId{3}, .payload = bytesOf(7)};

    EXPECT_EQ(packet, (Packet{.from = PeerId{3}, .payload = bytesOf(7)}));
    EXPECT_NE(packet, (Packet{.from = PeerId{4}, .payload = bytesOf(7)}));
    EXPECT_NE(packet, (Packet{.from = PeerId{3}, .payload = bytesOf(8)}));
}

// A payload is bytes, so an empty one is a payload rather than nothing.
TEST(PacketTest, AnEmptyPayloadIsNotTheSameAsAOneBytePayload)
{
    const Packet empty{.from = PeerId{1}, .payload = {}};

    EXPECT_NE(empty, (Packet{.from = PeerId{1}, .payload = bytesOf(0)}));
    EXPECT_EQ(empty, (Packet{.from = PeerId{1}, .payload = {}}));
}
