#include "antwika/tower_defence/Campaign.hpp"

#include <utility>

#include <antwika/rng/SplitMix64Rng.hpp>

namespace antwika::tower_defence
{

    namespace
    {
        using antwika::rng::SplitMix64Rng;

        // How far apart one level's seeds are spaced from the next.
        // Both are odd constants splitmix64 itself steps by.
        // So two levels start far apart in the sequence.
        // Two strides keep a layout and a wave order apart.
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

        // A battle nobody fights, for a campaign with no levels.
        // Campaign::battle() has to answer something.
        // An empty level with no waves is what nothing at all is.
        Battle emptyBattle()
        {
            return Battle(Level{}, BattleConfig{}, {});
        }
    } // namespace

    std::vector<LevelPlan> campaignLevels()
    {
        // Three levels that differ in how they play.
        // Not only in where the road happens to run.
        //
        // The approach is open ground, reaching two cells for one hit.
        // A corner covers a lot of it and everything dies to patience.
        // The narrows wall every other column and reach one cell only.
        // So a tower there has to hug the road to do anything at all.
        // Its guns hit for two, which is what lets armour exist here.
        // The gauntlet walls every fourth column, leaving long runs.
        // It gives the reach back, so a tower on one covers most of it.
        // What arrives there is heavy enough to need that.
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
                        // The last initialiser unwinds the rest.
                        // Only allocation failure takes that edge.
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

        // The closing brace is the members' landing pad.
        // It destroys the config and the battle if the body throws.
        // Only an allocation failure can take it.
    } // GCOVR_EXCL_LINE

    Battle Campaign::buildBattle(const std::size_t index) const
    {
        const LevelPlan &plan = config.levels[index];

        LevelConfig level = plan.level;
        level.seed = seedFor(config.seed, index, kLevelStride);

        // Drawn here, once, rather than inside the tick path.
        // What comes out is a fixed order the battle walks through.
        // So nothing a tick does depends on a generator's position.
        SplitMix64Rng rng(seedFor(config.seed, index, kWaveStride));

        // The campaign's mobs rather than the plan's default.
        // So one campaign plays one set of them.
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

        // The lives floor at nothing rather than wrapping.
        // Running out ends the campaign on the tick it happens.
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

} // namespace antwika::tower_defence
