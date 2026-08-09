#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "antwika/tower_defence/Battle.hpp"
#include "antwika/tower_defence/LevelGenerator.hpp"
#include "antwika/tower_defence/Wave.hpp"

namespace antwika::tower_defence
{

    enum class CampaignPhase : std::uint8_t
    {
        Fighting,

        Won,

        Lost,
    };

    [[nodiscard]] constexpr CampaignPhase enumBound(CampaignPhase) noexcept
    {
        return CampaignPhase::Lost;
    }

    struct LevelPlan final
    {
        LevelConfig level;

        BattleConfig battle;

        std::vector<Wave> waves;
    };

    inline constexpr std::uint32_t kStartingLives = 12;

    [[nodiscard]] std::vector<LevelPlan> campaignLevels();

    struct CampaignConfig final
    {
        std::uint64_t seed = 0;

        std::uint32_t lives = kStartingLives;

        std::vector<LevelPlan> levels = campaignLevels();

        std::array<MobProfile, kMobKindCount> mobs = kDefaultMobProfiles;
    };

    struct CampaignMemory final
    {
        std::size_t level = 0;

        std::uint64_t score = 0;

        std::uint32_t lives = 0;

        std::uint64_t ticks = 0;

        CampaignPhase phase = CampaignPhase::Fighting;

        BattleMemory battle;

        [[nodiscard]] bool operator==(const CampaignMemory &) const
            = default;
    };

    class Campaign final
    {
    public:
        explicit Campaign(CampaignConfig config);

        bool placeTower(const Cell &cell);

        void step();

        [[nodiscard]] const Battle &battle() const;

        [[nodiscard]] std::size_t levelIndex() const;

        [[nodiscard]] std::size_t levelCount() const;

        [[nodiscard]] std::uint64_t score() const;

        [[nodiscard]] std::uint32_t lives() const;

        [[nodiscard]] std::uint64_t ticks() const;

        [[nodiscard]] CampaignPhase phase() const;

        [[nodiscard]] CampaignMemory remember() const;

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

}
