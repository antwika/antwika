#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/Wave.hpp"

using antwika::rng::SplitMix64Rng;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::planWaves;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;
using antwika::tower_defence::WaveRelease;
using antwika::tower_defence::waveSize;

namespace
{
    std::map<MobKind, std::size_t> countsIn(
        const std::vector<MobKind> &order)
    {
        std::map<MobKind, std::size_t> counts;
        for (const MobKind kind : order)
        {
            ++counts[kind];
        }
        return counts;
    }

    Wave mixed()
    {
        return Wave{
            .entries =
                {WaveEntry{MobKind::Grunt, 4},
                 WaveEntry{MobKind::Runner, 3},
                 WaveEntry{MobKind::Brute, 1}},
            .spawnPeriodTicks = 5,
            .gapTicks = 9};
    }

    TEST(WaveTest, WaveSize_IsTheSumOfItsEntries)
    {
        EXPECT_EQ(waveSize(mixed()), 8U);
        EXPECT_EQ(waveSize(Wave{}), 0U);
    }

    TEST(WaveTest, PlanWaves_KeepsEveryMobAndThePacing)
    {
        SplitMix64Rng rng(1);
        const auto planned = planWaves({mixed()}, rng);

        ASSERT_EQ(planned.size(), 1U);
        EXPECT_EQ(planned[0].order.size(), 8U);
        EXPECT_EQ(planned[0].spawnPeriodTicks, 5U);
        EXPECT_EQ(planned[0].gapTicks, 9U);

        const auto counts = countsIn(planned[0].order);
        EXPECT_EQ(counts.at(MobKind::Grunt), 4U);
        EXPECT_EQ(counts.at(MobKind::Runner), 3U);
        EXPECT_EQ(counts.at(MobKind::Brute), 1U);
        EXPECT_EQ(counts.count(MobKind::Shielded), 0U);
    }

    TEST(WaveTest, PlanWaves_DealsTheRecordedOrderForItsSeed)
    {
        SplitMix64Rng rng(42);

        EXPECT_EQ(
            planWaves({mixed()}, rng)[0].order,
            (std::vector<MobKind>{
                MobKind::Grunt,
                MobKind::Grunt,
                MobKind::Runner,
                MobKind::Grunt,
                MobKind::Runner,
                MobKind::Grunt,
                MobKind::Brute,
                MobKind::Runner}));
    }

    TEST(WaveTest, PlanWaves_MayDealDifferentlyBySeed)
    {
        SplitMix64Rng reference(0);
        const auto expected = planWaves({mixed()}, reference)[0].order;

        bool sawADifferentOrder = false;
        for (std::uint64_t seed = 1; seed < 20; ++seed)
        {
            SplitMix64Rng rng(seed);
            sawADifferentOrder = sawADifferentOrder
                || planWaves({mixed()}, rng)[0].order != expected;
        }
        EXPECT_TRUE(sawADifferentOrder);
    }

    TEST(WaveTest, PlanWaves_SwapsNothingForNoneOrOne)
    {
        SplitMix64Rng rng(7);
        const auto planned = planWaves(
            {Wave{.entries = {}},
             Wave{.entries = {WaveEntry{MobKind::Shielded, 1}}}},
            rng);

        ASSERT_EQ(planned.size(), 2U);
        EXPECT_TRUE(planned[0].order.empty());
        EXPECT_EQ(
            planned[1].order,
            (std::vector<MobKind>{MobKind::Shielded}));
    }

    TEST(WaveTest, ProfileOf_BalancesEveryKind)
    {
        for (const MobKind kind : antwika::tower_defence::kAllMobKinds)
        {
            const auto profile = antwika::tower_defence::profileOf(kind);
            EXPECT_GE(profile.ticksPerCell, 1U);
            EXPECT_GT(profile.health, 0);
            EXPECT_GE(profile.armour, 0);
            EXPECT_GT(profile.reward, 0U);
        }
    }
}
