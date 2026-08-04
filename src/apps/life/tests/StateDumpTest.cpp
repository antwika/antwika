#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include <nlohmann/json.hpp>

#include "antwika/life/StateDump.hpp"

using antwika::life::Board;
using antwika::life::CellCoordinate;
using antwika::life::StateDump;
using antwika::life::StateDumpError;

namespace
{
    [[nodiscard]] StateDump wholeDump()
    {
        StateDump dump;

        dump.board = Board{
            .width = 3,
            .height = 2,
            .alive = {true, false, false, true, true, false}};
        dump.dragging = true;
        dump.visited = {
            CellCoordinate{.x = 0, .y = 0},
            CellCoordinate{.x = 2, .y = 1}};
        dump.lastDrag = antwika::input::Position{.x = -4, .y = 17};

        return dump;
    }
} // namespace

TEST(StateDumpTest, AWholeDumpSurvivesTheRoundTrip)
{
    const auto dump = wholeDump();

    const auto decoded =
        antwika::life::stateDumpFromJson(
            antwika::life::stateDumpToJson(dump));

    EXPECT_EQ(decoded, dump);
}

TEST(StateDumpTest, ADumpWithNoDragSurvivesTheRoundTrip)
{
    StateDump dump;
    dump.board = Board{
        .width = 2, .height = 2, .alive = {false, true, false, false}};

    const auto encoded = antwika::life::stateDumpToJson(dump);

    // No drag means no member, not a member holding nothing.
    EXPECT_FALSE(encoded.contains("lastDrag"));

    const auto decoded = antwika::life::stateDumpFromJson(encoded);

    EXPECT_EQ(decoded, dump);
    EXPECT_FALSE(decoded.dragging);
    EXPECT_TRUE(decoded.visited.empty());
    EXPECT_EQ(decoded.lastDrag, std::nullopt);
}

TEST(StateDumpTest, TheCellsEncodeAsZerosAndOnes)
{
    const auto encoded =
        antwika::life::stateDumpToJson(wholeDump());

    EXPECT_EQ(
        encoded.at("board").at("cells").get<std::string>(), "100110");
}

TEST(StateDumpTest, ADocumentOfTheWrongShapeIsRefused)
{
    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(
            nlohmann::json{{"board", "not an object"}}),
        StateDumpError);
}

TEST(StateDumpTest, ACellStringOfTheWrongLengthIsRefused)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["board"]["cells"] = "10011";

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, ACellCharacterThatIsNotABitIsRefused)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["board"]["cells"] = "10011x";

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, AVisitedColumnOffTheBoardIsRefused)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["visited"][0]["x"] = 3;

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, AVisitedRowOffTheBoardIsRefused)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["visited"][0]["y"] = 2;

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, TheMigrationsStartAndEndAtVersionOne)
{
    const auto chain = antwika::life::standardStateDumpMigrations();

    EXPECT_EQ(
        chain.currentVersion(), antwika::life::kStateDumpVersion);
}
