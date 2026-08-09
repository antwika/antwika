#include <gtest/gtest.h>

#include <optional>
#include <stdexcept>
#include <string>

#include <antwika/enums/FromName.hpp>
#include <antwika/enums/NameTable.hpp>

#include "Colour.hpp"

using antwika::enums::fromName;
using antwika::enums::NameTable;
using antwika::enums::orThrow;
using antwika::enums::tests::Colour;

namespace
{
    class Refused final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

    constexpr NameTable<Colour> kColours{{"red", "green", "blue"}};
}

TEST(FromNameTest, OrThrow_AnswersTheValueTheLookupFound)
{
    EXPECT_EQ(orThrow<Refused>(std::optional{7}, "no such: ", "7"), 7);
}

TEST(FromNameTest, OrThrow_RefusesALookupThatFoundNothing)
{
    EXPECT_THROW(
        static_cast<void>(
            orThrow<Refused>(std::optional<int>{}, "no such: ", "7")),
        Refused);
}

TEST(FromNameTest, OrThrow_NamesWhatFailedInTheMessage)
{
    try
    {
        static_cast<void>(
            orThrow<Refused>(std::optional<int>{}, "no such: ", "7"));
        FAIL();
    }
    catch (const Refused &refused)
    {
        EXPECT_EQ(std::string(refused.what()), "no such: 7");
    }
}

TEST(FromNameTest, FromName_ReadsAnEnumeratorTheTableNames)
{
    EXPECT_EQ(
        fromName<Refused>(kColours, "blue", "no such colour: "),
        Colour::Blue);
}

TEST(FromNameTest, FromName_RefusesANameTheTableLacks)
{
    EXPECT_THROW(
        static_cast<void>(
            fromName<Refused>(kColours, "puce", "no such colour: ")),
        Refused);
}
