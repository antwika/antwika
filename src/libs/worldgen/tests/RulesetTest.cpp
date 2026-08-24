#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

#include <antwika/voxel/VoxelCell.hpp>
#include <antwika/worldgen/ChunkShape.hpp>
#include <antwika/worldgen/Ruleset.hpp>
#include <antwika/worldgen/WorldgenError.hpp>
#include <antwika/worldgen/ruleset/CompiledRuleset.hpp>

using antwika::voxel::Facing;
using antwika::voxel::Kind;
using antwika::worldgen::Axis;
using antwika::worldgen::ChunkShape;
using antwika::worldgen::CompiledRuleset;
using antwika::worldgen::District;
using antwika::worldgen::Face;
using antwika::worldgen::faultsIn;
using antwika::worldgen::isDemand;
using antwika::worldgen::maskOf;
using antwika::worldgen::matesAcross;
using antwika::worldgen::matesUpright;
using antwika::worldgen::Prototype;
using antwika::worldgen::Role;
using antwika::worldgen::Ruleset;
using antwika::worldgen::RulesetFault;
using antwika::worldgen::Socket;
using antwika::worldgen::WorldgenError;

namespace
{
    Ruleset getPlainRuleset()
    {
        Ruleset ruleset;

        ruleset.prototypes = {
            Prototype{
                .name = "air open",
                .air = true,
                .sockets =
                    {Socket::OpenSide,
                     Socket::OpenSide,
                     Socket::Sky,
                     Socket::Floats,
                     Socket::OpenSide,
                     Socket::OpenSide},
                .roles = maskOf(Role::Perch)},
            Prototype{
                .name = "air room",
                .air = true,
                .sockets =
                    {Socket::RoomSide,
                     Socket::RoomSide,
                     Socket::Sky,
                     Socket::Stands,
                     Socket::RoomSide,
                     Socket::RoomSide},
                .roles = static_cast<std::uint8_t>(
                    maskOf(Role::Room) | maskOf(Role::Perch))},
            Prototype{
                .name = "stone",
                .sockets =
                    {Socket::Facade,
                     Socket::Facade,
                     Socket::Carries,
                     Socket::Rests,
                     Socket::Facade,
                     Socket::Facade},
                .roles = static_cast<std::uint8_t>(
                    maskOf(Role::Bear) | maskOf(Role::Land))},
            Prototype{
                .name = "step",
                .kind = Kind::Ramp,
                .facing = Facing::East,
                .sockets =
                    {Socket::Facade,
                     Socket::Facade,
                     Socket::Carries,
                     Socket::Rests,
                     Socket::Facade,
                     Socket::Facade},
                .roles = maskOf(Role::Step)}};

        ruleset.districts = {
            District{
                .name = "all of it",
                .untilShare = 100,
                .desire = {4, 3, 2, 1}}};

        return ruleset;
    }

    [[nodiscard]] bool holdsFault(
        const Ruleset &ruleset, const RulesetFault fault)
    {
        const auto problems = faultsIn(ruleset);

        return std::ranges::any_of(
            problems,
            [fault](const auto &problem) { return problem.fault == fault; });
    }
}

TEST(RulesetTest, FaultsIn_SaysNothingOfARulesetABlockCanBeGrownBy)
{
    EXPECT_TRUE(faultsIn(getPlainRuleset()).empty());
}

TEST(RulesetTest, FaultsIn_NamesARulesetWithNoPrototypeInIt)
{
    const auto problems = faultsIn(Ruleset{});

    ASSERT_EQ(problems.size(), 1U);
    EXPECT_EQ(problems.front().fault, RulesetFault::NoPrototypes);
}

TEST(RulesetTest, FaultsIn_NamesARampWithNoWayAbout)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.prototypes[3].facing = Facing::Any;

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::RampWithoutFacing));
}

TEST(RulesetTest, FaultsIn_NamesAFaceNoPrototypeMayStandAgainst)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.prototypes[2].sockets[static_cast<std::size_t>(Face::East)] =
        Socket::NeedsLanding;

    const auto problems = faultsIn(ruleset);

    ASSERT_EQ(problems.size(), 1U);
    EXPECT_EQ(problems.front().fault, RulesetFault::FaceMeetsNothing);
    EXPECT_EQ(problems.front().prototypeIndex, 2U);
    EXPECT_EQ(problems.front().face, Face::East);
}

TEST(RulesetTest, FaultsIn_NamesATopNoBottomAnswers)
{
    Ruleset ruleset;
    ruleset.prototypes = {
        Prototype{
            .name = "unstood upon",
            .sockets =
                {Socket::OpenSide,
                 Socket::OpenSide,
                 Socket::Carries,
                 Socket::Floats,
                 Socket::OpenSide,
                 Socket::OpenSide},
            .roles = 0}};
    ruleset.districts = {District{.untilShare = 100, .desire = {1}}};

    const auto problems = faultsIn(ruleset);
    const auto foundProblem = std::ranges::find_if(
        problems,
        [](const auto &problem)
        {
            return problem.fault == RulesetFault::FaceMeetsNothing
                   && problem.face == Face::Up;
        });

    EXPECT_NE(foundProblem, problems.end());
}

TEST(RulesetTest, FaultsIn_NamesABottomNoTopCarries)
{
    Ruleset ruleset;
    ruleset.prototypes = {
        Prototype{
            .name = "adrift",
            .sockets =
                {Socket::OpenSide,
                 Socket::OpenSide,
                 Socket::Sky,
                 Socket::Submerged,
                 Socket::OpenSide,
                 Socket::OpenSide},
            .roles = 0}};
    ruleset.districts = {District{.untilShare = 100, .desire = {1}}};

    const auto problems = faultsIn(ruleset);
    const auto foundProblem = std::ranges::find_if(
        problems,
        [](const auto &problem)
        {
            return problem.fault == RulesetFault::FaceMeetsNothing
                   && problem.face == Face::Down;
        });

    EXPECT_NE(foundProblem, problems.end());
}

TEST(RulesetTest, FaultsIn_NamesARoleNoPrototypeWears)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.prototypes[3].roles = 0;

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::RoleWornByNoPrototype));
}

TEST(RulesetTest, FaultsIn_NamesARulesetWithNoDistrictInIt)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.districts.clear();

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::NoDistricts));
}

TEST(RulesetTest, FaultsIn_NamesDistrictsThatDoNotRiseToTheTop)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.districts.front().untilShare = 60;

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::DistrictsDoNotRise));

    ruleset.districts = {
        District{.untilShare = 60, .desire = {1, 1, 1, 1}},
        District{.untilShare = 60, .desire = {1, 1, 1, 1}}};

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::DistrictsDoNotRise));
}

TEST(RulesetTest, FaultsIn_NamesADistrictWantingOneThingForEveryPrototype)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.districts.front().desire = {1};

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::DistrictMissizes));
}

TEST(RulesetTest, FaultsIn_NamesADistrictThatWantsNothingAtAll)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.districts.front().desire = {0, 0, 0, 0};

    EXPECT_TRUE(holdsFault(ruleset, RulesetFault::DistrictAllowsNothing));
}

TEST(RulesetTest, MatesAcross_AnswersAlikeEitherWayAbout)
{
    EXPECT_TRUE(matesAcross(Socket::NeedsRoot, Socket::Facade));
    EXPECT_TRUE(matesAcross(Socket::Facade, Socket::NeedsRoot));
    EXPECT_FALSE(matesAcross(Socket::Buried, Socket::OpenSide));
}

TEST(RulesetTest, MatesAcross_RefusesTwoCorbelsRootedOnEachOther)
{
    EXPECT_FALSE(matesAcross(Socket::NeedsRoot, Socket::NeedsRoot));
}

TEST(RulesetTest, MatesUpright_IsNotAlikeEitherWayAbout)
{
    EXPECT_TRUE(matesUpright(Socket::Carries, Socket::Rests));
    EXPECT_FALSE(matesUpright(Socket::Rests, Socket::Carries));
}

TEST(RulesetTest, MatesUpright_RefusesASolidStandingOverAir)
{
    EXPECT_FALSE(matesUpright(Socket::Sky, Socket::Rests));
    EXPECT_TRUE(matesUpright(Socket::Sky, Socket::Hangs));
}

TEST(RulesetTest, IsDemand_NamesOnlyTheSocketsAskingToBeHeldUp)
{
    EXPECT_TRUE(isDemand(Socket::NeedsRoot));
    EXPECT_TRUE(isDemand(Socket::NeedsLanding));
    EXPECT_FALSE(isDemand(Socket::Facade));
    EXPECT_FALSE(isDemand(Socket::Buried));
    EXPECT_FALSE(isDemand(Socket::WaterSide));
}

TEST(RulesetTest, TableAlong_HoldsOneTableForAWayAboutRatherThanForALink)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());

    EXPECT_EQ(
        compiledRuleset.sharedTableAlong(Axis::Across).get(),
        compiledRuleset.sharedTableAlong(Axis::Across).get());
    EXPECT_NE(
        compiledRuleset.sharedTableAlong(Axis::Across).get(),
        compiledRuleset.sharedTableAlong(Axis::Upright).get());
}

TEST(RulesetTest, TableAlong_RefusesASolidAboveAir)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());

    EXPECT_FALSE(compiledRuleset.tableAlong(Axis::Upright).isCompatible(0, 2));
    EXPECT_TRUE(compiledRuleset.tableAlong(Axis::Upright).isCompatible(2, 1));
}

TEST(RulesetTest, CompiledRuleset_TurnsAwayARulesetWithFaultsInIt)
{
    EXPECT_THROW(CompiledRuleset(Ruleset{}), WorldgenError);
}

TEST(RulesetTest, Matching_NamesEveryPrototypeLaidOutAsAKind)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());

    const auto stones = compiledRuleset.getMatching(Kind::Normal, Facing::Any);
    ASSERT_EQ(stones.size(), 1U);
    EXPECT_EQ(stones.front(), 2U);

    EXPECT_EQ(compiledRuleset.getMatching(Kind::Ramp, Facing::East).size(), 1U);
    EXPECT_TRUE(compiledRuleset.getMatching(Kind::Ramp, Facing::West).empty());
    EXPECT_EQ(compiledRuleset.getMatching(Kind::Ramp, Facing::Any).size(), 1U);
}

TEST(RulesetTest, Wearing_NamesEveryPrototypeWearingARole)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());

    ASSERT_EQ(compiledRuleset.getWearing(Role::Bear).size(), 1U);
    EXPECT_EQ(compiledRuleset.getWearing(Role::Bear).front(), 2U);
    EXPECT_TRUE(compiledRuleset.wears(2, Role::Land));
    EXPECT_FALSE(compiledRuleset.wears(0, Role::Land));
}

TEST(RulesetTest, At_TurnsAwayAPrototypeTheRulesetHasNot)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());

    EXPECT_THROW(
        (void)compiledRuleset.getEntryAt(compiledRuleset.getSize()), WorldgenError);
    EXPECT_EQ(compiledRuleset.getSize(), 4U);
    EXPECT_EQ(compiledRuleset.getSource().prototypes.size(), 4U);
}

TEST(RulesetTest, DistrictOf_SharesTheHeightOutBetweenTheDistricts)
{
    Ruleset ruleset = getPlainRuleset();
    ruleset.districts = {
        District{.name = "low", .untilShare = 50, .desire = {1, 1, 1, 1}},
        District{.name = "high", .untilShare = 100, .desire = {1, 1, 1, 1}}};

    const CompiledRuleset compiledRuleset(ruleset);
    constexpr ChunkShape shape{.height = 8};

    EXPECT_EQ(compiledRuleset.districtOf(shape, 0), 0U);
    EXPECT_EQ(compiledRuleset.districtOf(shape, 3), 0U);
    EXPECT_EQ(compiledRuleset.districtOf(shape, 4), 1U);
    EXPECT_EQ(compiledRuleset.districtOf(shape, 7), 1U);
}

TEST(RulesetTest, DistrictOf_HoldsALevelOutsideTheChunkToTheNearestOne)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());
    constexpr ChunkShape shape{.height = 8};

    EXPECT_EQ(compiledRuleset.districtOf(shape, -4), 0U);
    EXPECT_EQ(compiledRuleset.districtOf(shape, 99), 0U);
    EXPECT_THROW(
        (void)compiledRuleset.districtOf(ChunkShape{.height = 0}, 0),
        WorldgenError);
}

TEST(RulesetTest, DesireIn_TurnsAwayADistrictTheRulesetHasNot)
{
    const CompiledRuleset compiledRuleset(getPlainRuleset());

    EXPECT_EQ(compiledRuleset.desireIn(0).size(), 4U);
    EXPECT_THROW((void)compiledRuleset.desireIn(1), WorldgenError);
}
