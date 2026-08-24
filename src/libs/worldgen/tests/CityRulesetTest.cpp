#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <utility>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/CityRuleset.hpp>
#include <antwika/worldgen/Ruleset.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

using antwika::voxel::Facing;
using antwika::voxel::Kind;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::CityDistrict;
using antwika::worldgen::CityPiece;
using antwika::worldgen::getCityRuleset;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::faultsIn;
using antwika::worldgen::indexOf;
using antwika::worldgen::kCityPieces;
using antwika::worldgen::Role;

TEST(CityRulesetTest, CityRuleset_HasNothingWrongWithIt)
{
    EXPECT_TRUE(faultsIn(getCityRuleset()).empty());
}

TEST(CityRulesetTest, CityRuleset_NamesAWayAboutForEveryRamp)
{
    for (const auto &prototype : getCityRuleset().prototypes)
    {
        if (prototype.kind == Kind::Ramp)
        {
            EXPECT_NE(prototype.facing, Facing::Any) << prototype.name;
        }
    }
}

TEST(CityRulesetTest, CityRuleset_LetsNothingMassiveStandInTheHeights)
{
    const auto ruleset = getCityRuleset();
    const auto &heights =
        ruleset.districts[static_cast<std::size_t>(CityDistrict::Heights)];

    EXPECT_EQ(heights.desire[indexOf(CityPiece::Fill)], 0U);
    EXPECT_EQ(heights.desire[indexOf(CityPiece::Cistern)], 0U);
    EXPECT_GT(heights.desire[indexOf(CityPiece::Wall)], 0U);
    EXPECT_GT(heights.desire[indexOf(CityPiece::Floor)], 0U);
}

TEST(CityRulesetTest, CityRuleset_PutsTheSlumsBelowTheTerraces)
{
    const auto ruleset = getCityRuleset();
    const auto &slums =
        ruleset.districts[static_cast<std::size_t>(CityDistrict::Slums)];
    const auto &heights =
        ruleset.districts[static_cast<std::size_t>(CityDistrict::Heights)];

    EXPECT_LT(slums.untilShare, heights.untilShare);
    EXPECT_LT(
        slums.desire[indexOf(CityPiece::AirOpen)],
        heights.desire[indexOf(CityPiece::AirOpen)]);
    EXPECT_GT(
        slums.desire[indexOf(CityPiece::StairEast)],
        heights.desire[indexOf(CityPiece::StairEast)]);
}

TEST(CityRulesetTest, CityRuleset_WantsMostlyRockAtTheFootOfTheCity)
{
    const auto ruleset = getCityRuleset();
    const auto &bedrock =
        ruleset.districts[static_cast<std::size_t>(CityDistrict::Bedrock)];

    for (std::size_t index = 0; index < kCityPieces; ++index)
    {
        if (index == indexOf(CityPiece::Bedrock))
        {
            continue;
        }

        EXPECT_LT(
            bedrock.desire[index],
            bedrock.desire[indexOf(CityPiece::Bedrock)])
            << ruleset.prototypes[index].name;
    }
}

TEST(CityRulesetTest, CityRuleset_LetsACellarBeCutIntoTheRock)
{
    const auto ruleset = getCityRuleset();
    const auto &bedrock =
        ruleset.districts[static_cast<std::size_t>(CityDistrict::Bedrock)];

    EXPECT_GT(bedrock.desire[indexOf(CityPiece::AirRoom)], 0U);
    EXPECT_GT(bedrock.desire[indexOf(CityPiece::StairEast)], 0U);
    EXPECT_GT(bedrock.desire[indexOf(CityPiece::Cistern)], 0U);
}

TEST(CityRulesetTest, CityRuleset_HoldsAPieceForEveryRole)
{
    const CompiledRuleset compiledRuleset(getCityRuleset());

    EXPECT_EQ(compiledRuleset.getSize(), kCityPieces);
    EXPECT_FALSE(compiledRuleset.getWearing(Role::Room).empty());
    EXPECT_FALSE(compiledRuleset.getWearing(Role::Perch).empty());
    EXPECT_FALSE(compiledRuleset.getWearing(Role::Bear).empty());
    EXPECT_FALSE(compiledRuleset.getWearing(Role::Step).empty());
    EXPECT_FALSE(compiledRuleset.getWearing(Role::Land).empty());
}

TEST(CityRulesetTest, CityRuleset_GivesTheDistrictsTheirOwnBandsOfHeight)
{
    const CompiledRuleset compiledRuleset(getCityRuleset());
    constexpr ChunkShape shape{};

    EXPECT_EQ(
        compiledRuleset.districtOf(shape, 0),
        static_cast<std::size_t>(CityDistrict::Bedrock));
    EXPECT_EQ(
        compiledRuleset.districtOf(shape, shape.height - 1),
        static_cast<std::size_t>(CityDistrict::Sky));
    EXPECT_LT(
        compiledRuleset.districtOf(shape, 12),
        compiledRuleset.districtOf(shape, 50));
}

TEST(CityRulesetTest, CityRuleset_MatchesEveryPaintedCubeToSomePiece)
{
    const CompiledRuleset compiledRuleset(getCityRuleset());

    EXPECT_FALSE(compiledRuleset.getMatching(Kind::Normal, Facing::Any).empty());
    EXPECT_FALSE(compiledRuleset.getMatching(Kind::Water, Facing::Any).empty());
    EXPECT_EQ(compiledRuleset.getMatching(Kind::Ramp, Facing::Any).size(), 4U);
    EXPECT_EQ(compiledRuleset.getMatching(Kind::Ramp, Facing::East).size(), 1U);
}

TEST(CityRulesetTest, CityRuleset_ListsItsPiecesAsTheEnumNamesThem)
{
    constexpr std::array<
        std::pair<CityPiece, std::string_view>, kCityPieces>
        pieceNames{{
            {CityPiece::AirOpen, "air open"},
            {CityPiece::AirRoom, "air room"},
            {CityPiece::Bedrock, "bedrock"},
            {CityPiece::Fill, "fill"},
            {CityPiece::Wall, "wall"},
            {CityPiece::Floor, "floor"},
            {CityPiece::CorbelEast, "corbel east"},
            {CityPiece::CorbelWest, "corbel west"},
            {CityPiece::CorbelNorth, "corbel north"},
            {CityPiece::CorbelSouth, "corbel south"},
            {CityPiece::StairEast, "stair east"},
            {CityPiece::StairWest, "stair west"},
            {CityPiece::StairNorth, "stair north"},
            {CityPiece::StairSouth, "stair south"},
            {CityPiece::Cistern, "cistern"}}};

    const auto ruleset = getCityRuleset();

    ASSERT_EQ(ruleset.prototypes.size(), kCityPieces);

    for (const auto &[piece, name] : pieceNames)
    {
        EXPECT_EQ(ruleset.prototypes[indexOf(piece)].name, name);
    }
}
