#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <vector>

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
}

TEST(StateDumpTest, StateDumpFromJson_KeepsAWholeDump)
{
    const auto dump = wholeDump();

    const auto decoded =
        antwika::life::stateDumpFromJson(
            antwika::life::stateDumpToJson(dump));

    EXPECT_EQ(decoded, dump);
}

TEST(StateDumpTest, StateDumpFromJson_KeepsADumpWithNoDrag)
{
    StateDump dump;
    dump.board = Board{
        .width = 2, .height = 2, .alive = {false, true, false, false}};

    const auto encoded = antwika::life::stateDumpToJson(dump);

    EXPECT_FALSE(encoded.contains("lastDrag"));

    const auto decoded = antwika::life::stateDumpFromJson(encoded);

    EXPECT_EQ(decoded, dump);
    EXPECT_FALSE(decoded.dragging);
    EXPECT_TRUE(decoded.visited.empty());
    EXPECT_EQ(decoded.lastDrag, std::nullopt);
}

TEST(StateDumpTest, StateDumpToJson_EncodesCellsAsZerosAndOnes)
{
    const auto encoded =
        antwika::life::stateDumpToJson(wholeDump());

    EXPECT_EQ(
        encoded.at("board").at("cells").get<std::string>(), "100110");
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAWrongShape)
{
    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(
            nlohmann::json{{"board", "not an object"}}),
        StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAWrongCellLength)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["board"]["cells"] = "10011";

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesANonBitCell)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["board"]["cells"] = "10011x";

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesAColumnOffTheBoard)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["visited"][0]["x"] = 3;

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, StateDumpFromJson_RefusesARowOffTheBoard)
{
    auto encoded = antwika::life::stateDumpToJson(wholeDump());
    encoded["visited"][0]["y"] = 2;

    EXPECT_THROW(
        (void)antwika::life::stateDumpFromJson(encoded), StateDumpError);
}

TEST(StateDumpTest, StandardStateDumpMigrations_StartAndEndAtOne)
{
    const auto chain = antwika::life::standardStateDumpMigrations();

    EXPECT_EQ(chain.currentVersion(), 1U);
    EXPECT_EQ(antwika::life::kStateDumpVersion, 1U);
}

TEST(StateDumpTest, OperatorEquals_ComparesEveryField)
{
    const auto base = wholeDump();

    const auto twin = base;
    EXPECT_EQ(base, twin);

    auto reboarded = base;
    reboarded.board.alive[0] = false;
    EXPECT_NE(base, reboarded);

    auto released = base;
    released.dragging = false;
    EXPECT_NE(base, released);

    auto unvisited = base;
    unvisited.visited.clear();
    EXPECT_NE(base, unvisited);

    auto rested = base;
    rested.lastDrag.reset();
    EXPECT_NE(base, rested);
}
