#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/LevelGenerator.hpp"
#include "antwika/tower_defence/Wave.hpp"

namespace antwika::tower_defence
{

    /** @brief How far a campaign has got. */
    enum class CampaignPhase : std::uint8_t
    {
        /** @brief A level is being fought. */
        Fighting,

        /** @brief The last wave of the last level was cleared. */
        Won,

        /** @brief The lives ran out. */
        Lost,
    };

    /**
     * @brief One level of a campaign, as its designer states it.
     *
     * A level is not only a different layout: the grid it is solved
     * over, how densely it is walled, what a tower reaches, what a tower
     * hits for and what walks down it are all here, so two levels of the
     * shipped campaign play differently rather than looking different.
     */
    struct LevelPlan
    {
        /**
         * @brief The grid to generate.
         *
         * The seed is not read from here: Campaign derives one per level
         * from its own seed, so the whole campaign is reproducible from
         * one number.
         */
        LevelConfig level;

        /** @brief The numbers this level's guns are balanced with. */
        BattleConfig battle;

        /** @brief The waves to fight, in order. */
        std::vector<Wave> waves;
    };

    /** @brief Lives a campaign starts with, if nobody says otherwise. */
    inline constexpr std::uint32_t kStartingLives = 12;

    /**
     * @brief The shipped campaign, in the order it is played.
     *
     * A function rather than a constant because a LevelPlan holds
     * vectors, and a caller is free to hand Campaign a list of its own
     * -- which is how every test here fights a level it can describe in
     * three lines.
     *
     * @return The three levels, first to last.
     */
    [[nodiscard]] std::vector<LevelPlan> campaignLevels();

    /** @brief Everything one campaign is decided by. */
    struct CampaignConfig
    {
        /**
         * @brief Chooses every level's layout and every wave's order.
         *
         * One number for the whole campaign: each level's generator seed
         * and each level's wave shuffle are derived from it, so the same
         * seed gives the same campaign and a replay that carries no
         * level still lands on the one that was recorded.
         */
        std::uint64_t seed = 0;

        /** @brief Leaks the player can afford before it is over. */
        std::uint32_t lives = kStartingLives;

        /** @brief The levels to fight, first to last. */
        std::vector<LevelPlan> levels = campaignLevels();

        /**
         * @brief What each mob kind is worth, costs and survives.
         *
         * Handed to every level's battle, so one campaign plays one
         * set of mobs.
         */
        std::array<MobProfile, kMobKindCount> mobs = kDefaultMobProfiles;
    };

    /**
     * @brief Everything one campaign has moved, and nothing its seed
     * rebuilds.
     *
     * The level layouts and the wave orders are deliberately absent:
     * they are pure functions of the campaign's seed and the level
     * index, so Campaign::restore() regenerates them the way
     * Campaign::buildBattle() generated them the first time.
     */
    struct CampaignMemory
    {
        /** @brief Which level is being fought, counting from zero. */
        std::size_t level = 0;

        /** @brief Score earned across every level so far. */
        std::uint64_t score = 0;

        /** @brief Leaks the player can still afford. */
        std::uint32_t lives = 0;

        /** @brief How many ticks step() has been called for. */
        std::uint64_t ticks = 0;

        /** @brief How far the campaign has got. */
        CampaignPhase phase = CampaignPhase::Fighting;

        /** @brief The fight the level index rebuilds a level for. */
        BattleMemory battle;

        [[nodiscard]] bool operator==(const CampaignMemory &) const
            = default;
    };

    /**
     * @brief A sequence of levels, the score across them and the lives
     * left to spend.
     *
     * This is the simulation object the sinks hold, rather than the
     * Battle inside it: a battle is replaced wholesale when a level is
     * cleared, so anything holding one by reference would be left
     * pointing at the level before.
     *
     * Everything it does is integer and a pure function of its config
     * and the ticks stepped so far, and the only draw it makes -- the
     * level layouts and the wave orders -- happens in the constructor
     * and in advance(), from a generator seeded off config.seed.
     */
    class Campaign final
    {
    public:
        /**
         * @brief Start a campaign on its first level.
         * @param config What the campaign is decided by.
         * @throws LevelError If a level's config describes a grid no
         * level can exist in, or the solver ran out of attempts.
         */
        explicit Campaign(CampaignConfig config);

        /**
         * @brief Try to put a tower on a cell of the level being
         * fought.
         * @param cell Where to build.
         * @return True if a tower was built; false on a cell that is
         * unbuildable, and on any cell at all once the campaign is over.
         */
        bool placeTower(const Cell &cell);

        /**
         * @brief Advance the campaign by one tick.
         *
         * Steps the level being fought, spends a life per leak and adds
         * what the kills earned.
         * A cleared level is followed by the next one straight away, so
         * the tick that clears the last wave is the tick the next
         * level's first mob is released on.
         * A campaign that is over steps nothing, so its score is final.
         *
         * @throws LevelError If the next level cannot be generated.
         */
        void step();

        /** @brief The level being fought. */
        [[nodiscard]] const Battle &battle() const;

        /** @brief Which level that is, counting from zero. */
        [[nodiscard]] std::size_t levelIndex() const;

        /** @brief How many levels the campaign has. */
        [[nodiscard]] std::size_t levelCount() const;

        /** @brief Score earned across every level so far. */
        [[nodiscard]] std::uint64_t score() const;

        /** @brief Leaks the player can still afford. */
        [[nodiscard]] std::uint32_t lives() const;

        /** @brief How many ticks step() has been called for. */
        [[nodiscard]] std::uint64_t ticks() const;

        /** @brief How far the campaign has got. */
        [[nodiscard]] CampaignPhase phase() const;

        /**
         * @brief Take everything this campaign has moved.
         * @return The state its seed does not rebuild.
         */
        [[nodiscard]] CampaignMemory remember() const;

        /**
         * @brief Put a remembered campaign back, regenerating its
         * level.
         *
         * The battle is rebuilt from (config.seed, memory.level)
         * exactly as the constructor and step() build one, so the
         * level and the wave order come back without ever having been
         * written down.
         * A campaign standing one past its last level -- a won one --
         * gets the empty battle nobody fights, as a levelless config
         * does.
         *
         * @param memory What some campaign's remember() answered.
         * @return True when it fit; false for a level index past one
         * beyond the last level, and for a fight that does not fit the
         * regenerated level -- both refused rather than repaired,
         * leaving this campaign as it stood.
         * @throws LevelError If the named level cannot be generated.
         */
        [[nodiscard]] bool restore(const CampaignMemory &memory);

    private:
        CampaignConfig config;
        Battle current;
        std::size_t level = 0;
        std::uint64_t totalScore = 0;
        std::uint32_t livesLeft = 0;
        std::uint64_t tickCount = 0;
        CampaignPhase state = CampaignPhase::Fighting;

        [[nodiscard]] Battle buildBattle(std::size_t index) const;
    };

} // namespace antwika::tower_defence
