#include "BinaryPrimitives.hpp"

#include <cstdint>
#include <limits>
#include <sstream>
#include <string>

#include <gtest/gtest.h>

#include <antwika/replay/ReplayFormatError.hpp>

using antwika::replay::ReplayFormatError;
using antwika::replay::detail::checkFitsInU32Length;
using antwika::replay::detail::readString;
using antwika::replay::detail::readU32;
using antwika::replay::detail::readU64;
using antwika::replay::detail::writeString;
using antwika::replay::detail::writeU32;
using antwika::replay::detail::writeU64;

TEST(BinaryPrimitivesTest, U32RoundTripsAValue)
{
    std::stringstream stream;
    writeU32(0xDEADBEEF, stream);
    EXPECT_EQ(readU32(stream), 0xDEADBEEFU);
}

TEST(BinaryPrimitivesTest, U64RoundTripsAValue)
{
    std::stringstream stream;
    writeU64(0x0123456789ABCDEF, stream);
    EXPECT_EQ(readU64(stream), 0x0123456789ABCDEFULL);
}

TEST(BinaryPrimitivesTest, StringRoundTripsAValue)
{
    std::stringstream stream;
    writeString("hello", stream);
    EXPECT_EQ(readString(stream), "hello");
}

TEST(BinaryPrimitivesTest, CheckFitsInU32LengthAcceptsTheMaximum)
{
    EXPECT_NO_THROW(
        checkFitsInU32Length(std::numeric_limits<std::uint32_t>::max()));
}

TEST(BinaryPrimitivesTest, CheckFitsInU32LengthRejectsOneOverTheMaximum)
{
    const auto tooLarge =
        static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max()) +
        1;
    EXPECT_THROW(checkFitsInU32Length(tooLarge), ReplayFormatError);
}
