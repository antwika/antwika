#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <antwika/app/preview/DrawnPreview.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>

#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/BattleSnapshot.hpp"
#include "antwika/tower_defence/Campaign.hpp"

namespace
{
    using antwika::app::preview::drawnPreview;
    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::tower_defence::BattleConfig;
    using antwika::tower_defence::BattleScene;
    using antwika::tower_defence::Campaign;
    using antwika::tower_defence::CampaignConfig;
    using antwika::tower_defence::Cell;
    using antwika::tower_defence::LevelPlan;
    using antwika::tower_defence::MobKind;
    using antwika::tower_defence::snapshotOf;
    using antwika::tower_defence::Tile;
    using antwika::tower_defence::Wave;
    using antwika::tower_defence::WaveEntry;

    constexpr Size kCanvas{.width = 960, .height = 720};

    [[nodiscard]] CampaignConfig oneLevel()
    {
        return CampaignConfig{
            .seed = 5,
            .lives = 20,
            .levels = {LevelPlan{
                .level = {.width = 12, .height = 8, .wallSpacing = 3},
                .battle =
                    BattleConfig{
                        .towerRangeSquared = 4, .towerDamage = 1},
                .waves = {Wave{
                    .entries = {WaveEntry{MobKind::Grunt, 4}},
                    .spawnPeriodTicks = 4,
                    .gapTicks = 0}}}}};
    }

    void placeSomeTowers(Campaign &campaign)
    {
        const auto &level = campaign.battle().level();
        std::uint32_t placed = 0;

        for (std::uint32_t y = 0; y < level.height && placed < 4; ++y)
        {
            for (std::uint32_t x = 0; x < level.width && placed < 4; ++x)
            {
                const Cell cell{.x = x, .y = y};

                if (level.at(cell) == Tile::Empty
                    && campaign.placeTower(cell))
                {
                    ++placed;
                }
            }
        }
    }
}

TEST(BattlePreviewTest, Draw_WritesABattleUnderWay)
{
    EXPECT_FALSE(
        drawnPreview(
            {.name = "tower-defence",
             .title = "Antwika Tower Defence",
             .canvas = kCanvas},
            [](IRenderer &renderer)
            {
                Campaign campaign(oneLevel());
                placeSomeTowers(campaign);

                for (std::uint32_t step = 0; step < 20; ++step)
                {
                    campaign.step();
                }

                const BattleScene scene;
                scene.draw(renderer, kCanvas, snapshotOf(campaign));
            })
            .empty());
}
