#include "antwika/tower_defence/Campaign.hpp"

#include <utility>

#include <antwika/rng/SplitMix64Rng.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::rng::SplitMix64Rng;

        constexpr std::uint64_t kLevelStride = 0x9E3779B97F4A7C15ULL;
        constexpr std::uint64_t kWaveStride = 0xBF58476D1CE4E5B9ULL;

        std::uint64_t seedFor(
            const std::uint64_t campaign,
            const std::size_t index,
            const std::uint64_t stride)
        {
            return campaign
                ^ ((static_cast<std::uint64_t>(index) + 1) * stride);
        }

        Battle emptyBattle()
        {
            return Battle(Level{}, BattleConfig{}, {});
        }
    }

    std::vector<LevelPlan> campaignLevels()
    {
        return {
            LevelPlan{
                .level = {.width = 12, .height = 8, .wallSpacing = 3},
                .battle = {.towerRangeSquared = 4, .towerDamage = 1},
                .waves =
                    {Wave{
                         .entries = {{MobKind::Grunt, 4}},
                         .spawnPeriodTicks = 6,
                         .gapTicks = 30},
                     Wave{
                         .entries =
                             {{MobKind::Grunt, 4}, {MobKind::Runner, 2}},
                         .spawnPeriodTicks = 5,
                         .gapTicks = 30},
                     Wave{
                         .entries = {{MobKind::Runner, 6}},
                         .spawnPeriodTicks = 4,
                         .gapTicks = 0}}},
            LevelPlan{
                .level = {.width = 10, .height = 10, .wallSpacing = 2},
                .battle = {.towerRangeSquared = 2, .towerDamage = 2},
                .waves =
                    {Wave{
                         .entries = {{MobKind::Grunt, 6}},
                         .spawnPeriodTicks = 5,
                         .gapTicks = 30},
                     Wave{
                         .entries =
                             {{MobKind::Brute, 2}, {MobKind::Runner, 4}},
                         .spawnPeriodTicks = 5,
                         .gapTicks = 30},
                     Wave{
                         .entries =
                             {{MobKind::Shielded, 3},
                              {MobKind::Grunt, 4}},
                         .spawnPeriodTicks = 4,
                         .gapTicks = 0}}},
            LevelPlan{
                .level = {.width = 14, .height = 9, .wallSpacing = 4},
                .battle = {.towerRangeSquared = 8, .towerDamage = 2},
                .waves = {
                    Wave{
                        .entries =
                            {{MobKind::Brute, 3},
                             {MobKind::Shielded, 3}},
                        .spawnPeriodTicks = 4,
                        .gapTicks = 30},
                    Wave{
                        .entries =
                            {{MobKind::Runner, 8},
                             {MobKind::Shielded, 4}},
                        .spawnPeriodTicks = 3,
                        .gapTicks = 30},
                    Wave{
                        .entries =
                            {{MobKind::Grunt, 6},
                             {MobKind::Brute, 4},
                             {MobKind::Shielded, 4},
                             {MobKind::Runner, 6}},
                        .spawnPeriodTicks = 3,
                        .gapTicks = 0}}}}; // GCOVR_EXCL_LINE
    }

    Campaign::Campaign(CampaignConfig setup)
        : config(std::move(setup)),
          current(emptyBattle()),
          livesLeft(config.lives)
    {
        if (config.levels.empty())
        {
            state = CampaignPhase::Won;
            return;
        }

        current = buildBattle(0);

    } // GCOVR_EXCL_LINE

    Battle Campaign::buildBattle(const std::size_t index) const
    {
        const LevelPlan &plan = config.levels[index];

        LevelConfig level = plan.level;
        level.seed = seedFor(config.seed, index, kLevelStride);

        SplitMix64Rng rng(seedFor(config.seed, index, kWaveStride));

        BattleConfig battle = plan.battle;
        battle.mobs = config.mobs;

        return Battle(
            generateLevel(level), battle, planWaves(plan.waves, rng));
    }

    bool Campaign::placeTower(const Cell &cell)
    {
        if (state != CampaignPhase::Fighting)
        {
            return false;
        }

        return current.placeTower(cell);
    }

    void Campaign::step()
    {
        ++tickCount;

        if (state != CampaignPhase::Fighting)
        {
            return;
        }

        const StepOutcome outcome = current.step();
        totalScore += outcome.reward;

        if (outcome.leaks >= livesLeft)
        {
            livesLeft = 0;
            state = CampaignPhase::Lost;
            return;
        }
        livesLeft -= outcome.leaks;

        if (!current.cleared())
        {
            return;
        }

        ++level;
        if (level >= config.levels.size())
        {
            state = CampaignPhase::Won;
            return;
        }

        current = buildBattle(level);
    }

    const Battle &Campaign::battle() const
    {
        return current;
    }

    std::size_t Campaign::levelIndex() const
    {
        return level;
    }

    std::size_t Campaign::levelCount() const
    {
        return config.levels.size();
    }

    std::uint64_t Campaign::score() const
    {
        return totalScore;
    }

    std::uint32_t Campaign::lives() const
    {
        return livesLeft;
    }

    std::uint64_t Campaign::ticks() const
    {
        return tickCount;
    }

    CampaignPhase Campaign::phase() const
    {
        return state;
    }

    CampaignMemory Campaign::remember() const
    {
        return CampaignMemory{ // GCOVR_EXCL_LINE
            .level = level,
            .score = totalScore,
            .lives = livesLeft,
            .ticks = tickCount,
            .phase = state,
            .battle = current.remember()};
    } // GCOVR_EXCL_LINE

    bool Campaign::restore(const CampaignMemory &memory)
    {
        if (memory.level > config.levels.size())
        {
            return false;
        }

        Battle rebuilt = memory.level < config.levels.size()
            ? buildBattle(memory.level)
            : emptyBattle();

        if (!rebuilt.restore(memory.battle))
        {
            return false;
        }

        current = std::move(rebuilt);
        level = memory.level;
        totalScore = memory.score;
        livesLeft = memory.lives;
        tickCount = memory.ticks;
        state = memory.phase;
        return true;
    }

}
