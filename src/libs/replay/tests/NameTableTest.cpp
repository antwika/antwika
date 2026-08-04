#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "antwika/replay/NameTable.hpp"

using antwika::replay::NameTable;

namespace
{
    enum class Tool : std::uint8_t
    {
        Paint = 0,
        Erase,
        Fill,
    };

    constexpr NameTable<Tool, 3> kTools{{"paint", "erase", "fill"}};
} // namespace

TEST(NameTableTest, Name_AnswersWithTheNameAtTheValuesIndex)
{
    EXPECT_EQ(kTools.name(Tool::Paint), "paint");
    EXPECT_EQ(kTools.name(Tool::Erase), "erase");
    EXPECT_EQ(kTools.name(Tool::Fill), "fill");
}

// Total rather than undefined, which is what the modulo is for.
// Reading past the array is not the way to report a bad number.
TEST(NameTableTest, Name_StaysInsideTheTableForAValueOffTheEnd)
{
    EXPECT_EQ(kTools.name(static_cast<Tool>(3)), "paint");
    EXPECT_EQ(kTools.name(static_cast<Tool>(5)), "fill");
    EXPECT_EQ(kTools.name(static_cast<Tool>(255)), "paint");
}

TEST(NameTableTest, From_AnswersWithTheValueANameGoesBy)
{
    EXPECT_EQ(kTools.from("paint"), Tool::Paint);
    EXPECT_EQ(kTools.from("erase"), Tool::Erase);
    EXPECT_EQ(kTools.from("fill"), Tool::Fill);
}

TEST(NameTableTest, From_AnswersWithNothingForANameNothingGoesBy)
{
    EXPECT_FALSE(kTools.from("smudge").has_value());
    EXPECT_FALSE(kTools.from("").has_value());
}

// A std::string reads as the string_view the lookup takes.
// Which is what every decode here hands it.
TEST(NameTableTest, From_ReadsAStringAsWellAsAView)
{
    EXPECT_EQ(kTools.from(std::string("erase")), Tool::Erase);
}

// Both directions are usable where a constant is required.
TEST(NameTableTest, NameAndFrom_AreBothConstantExpressions)
{
    static_assert(kTools.name(Tool::Fill) == "fill");
    static_assert(kTools.from("fill") == Tool::Fill);
    static_assert(!kTools.from("smudge").has_value());
}
