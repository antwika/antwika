#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/Wave.hpp"

using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::campaignLevels;
using antwika::tower_defence::CampaignPhase;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::profileOf;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;

namespace
{
    LevelPlan tinyLevel(std::vector<Wave> waves)
    {
        return LevelPlan{
            .level = {.width = 5, .height = 3, .wallSpacing = 3},
            .battle = BattleConfig{},
            .waves = std::move(waves)};
    }

    Wave oneGrunt()
    {
        return Wave{
            .entries = {WaveEntry{MobKind::Grunt, 1}},
            .spawnPeriodTicks = 1,
            .gapTicks = 0};
    }

    void runOut(Campaign &campaign, const int cap = 400)
    {
        for (int step = 0; step < cap; ++step)
        {
            if (campaign.phase() != CampaignPhase::Fighting)
            {
                return;
            }
            campaign.step();
        }
    }

    TEST(CampaignTest, CampaignLevels_DifferFromOneAnother)
    {
        const auto levels = campaignLevels();
        ASSERT_GE(levels.size(), 3U);

        for (std::size_t i = 1; i < levels.size(); ++i)
        {
            for (std::size_t j = 0; j < i; ++j)
            {
                const bool sameGrid =
                    levels[i].level.width == levels[j].level.width
                    && levels[i].level.height == levels[j].level.height
                    && levels[i].level.wallSpacing
                        == levels[j].level.wallSpacing;
                const bool sameGuns =
                    levels[i].battle.towerRangeSquared
                        == levels[j].battle.towerRangeSquared
                    && levels[i].battle.towerDamage
                        == levels[j].battle.towerDamage;
                EXPECT_FALSE(sameGrid && sameGuns) << j << ' ' << i;
            }
        }

        for (const LevelPlan &plan : levels)
        {
            EXPECT_GE(plan.waves.size(), 2U);
            for (const Wave &wave : plan.waves)
            {
                EXPECT_FALSE(wave.entries.empty());
            }
        }
    }

    TEST(CampaignTest, CampaignLevels_FieldNoUnkillableKind)
    {
        for (const LevelPlan &plan : campaignLevels())
        {
            for (const Wave &wave : plan.waves)
            {
                for (const WaveEntry &entry : wave.entries)
                {
                    EXPECT_GT(
                        plan.battle.towerDamage,
                        profileOf(entry.kind).armour)
                        << static_cast<int>(entry.kind);
                }
            }
        }
    }

    TEST(CampaignTest, Step_FightsEveryLevelInTurn)
    {
        Campaign campaign(CampaignConfig{
            .seed = 5,
            .lives = 20,
            .levels = {tinyLevel({oneGrunt()}), tinyLevel({oneGrunt()})}});

        EXPECT_EQ(campaign.levelIndex(), 0U);
        EXPECT_EQ(campaign.levelCount(), 2U);
        EXPECT_EQ(campaign.phase(), CampaignPhase::Fighting);
        EXPECT_EQ(campaign.battle().waveCount(), 1U);

        while (campaign.levelIndex() == 0
               && campaign.phase() == CampaignPhase::Fighting)
        {
            campaign.step();
        }
        EXPECT_EQ(campaign.levelIndex(), 1U);
        EXPECT_EQ(campaign.phase(), CampaignPhase::Fighting);

        runOut(campaign);
        EXPECT_EQ(campaign.phase(), CampaignPhase::Won);
        EXPECT_EQ(campaign.lives(), 18U);
    }

    TEST(CampaignTest, Step_EndsAndFreezesTheScoreOnNoLives)
    {
        Campaign campaign(CampaignConfig{
            .seed = 5,
            .lives = 1,
            .levels = {tinyLevel({oneGrunt()}), tinyLevel({oneGrunt()})}});

        runOut(campaign);
        EXPECT_EQ(campaign.phase(), CampaignPhase::Lost);
        EXPECT_EQ(campaign.lives(), 0U);
        EXPECT_EQ(campaign.levelIndex(), 0U);

        const std::uint64_t frozen = campaign.score();
        const std::uint64_t battleTicks = campaign.battle().ticks();
        const std::uint64_t ticks = campaign.ticks();
        EXPECT_FALSE(campaign.placeTower({.x = 1, .y = 1}));
        campaign.step();
        EXPECT_EQ(campaign.score(), frozen);
        EXPECT_EQ(campaign.battle().ticks(), battleTicks);
        EXPECT_TRUE(campaign.battle().towers().empty());

        EXPECT_EQ(campaign.ticks(), ticks + 1);
    }

    TEST(CampaignTest, Phase_IsWonWithNoLevels)
    {
        Campaign campaign(CampaignConfig{.levels = {}});
        EXPECT_EQ(campaign.phase(), CampaignPhase::Won);
        EXPECT_EQ(campaign.levelCount(), 0U);
        EXPECT_TRUE(campaign.battle().level().path.empty());
        EXPECT_FALSE(campaign.placeTower({.x = 0, .y = 0}));
    }

    TEST(CampaignTest, Step_ScoresTheRecordedRunForItsSeed)
    {
        const auto play = [](const std::uint64_t seed)
        {
            Campaign campaign(CampaignConfig{
                .seed = seed,
                .lives = 20,
                .levels = {
                    tinyLevel({oneGrunt()}), tinyLevel({oneGrunt()})}});
            static_cast<void>(campaign.placeTower({.x = 2, .y = 2}));
            runOut(campaign);
            return campaign.score();
        };

        EXPECT_EQ(play(11), 10);
    }

    TEST(CampaignTest, Step_VariesTheLevelBySeed)
    {
        const auto tiles = [](const std::uint64_t seed)
        {
            const Campaign campaign(CampaignConfig{
                .seed = seed, .levels = {tinyLevel({oneGrunt()})}});
            return campaign.battle().level().tiles;
        };

        const auto reference = tiles(0);
        bool sawADifferentLevel = false;
        for (std::uint64_t seed = 1; seed < 12; ++seed)
        {
            sawADifferentLevel =
                sawADifferentLevel || tiles(seed) != reference;
        }
        EXPECT_TRUE(sawADifferentLevel);
    }
}
