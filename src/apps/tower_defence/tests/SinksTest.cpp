#include <chrono>
#include <cstdint>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/mocks/MockRenderer.hpp>
#include <antwika/gfx/mocks/MockWindow.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Translator.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputEventCodec.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/time/fakes/FakeSleeper.hpp>

#include "antwika/tower_defence/BattleScene.hpp"
#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/CampaignSink.hpp"
#include "antwika/tower_defence/GridLayout.hpp"
#include "antwika/tower_defence/Level.hpp"
#include "antwika/tower_defence/LevelTile.hpp"
#include "antwika/tower_defence/MobKind.hpp"
#include "antwika/tower_defence/RenderSink.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"
#include "antwika/tower_defence/ScoreSink.hpp"
#include "antwika/tower_defence/TowerPlacementSink.hpp"

using antwika::event::Event;
using antwika::event::TickEvent;
using antwika::gfx::mocks::MockRenderer;
using antwika::gfx::mocks::MockWindow;
using antwika::gfx::Size;
using antwika::i18n::Translator;
using antwika::input::InputEventCodec;
using antwika::input::MouseButton;
using antwika::input::PointerButtonPressed;
using antwika::input::PointerButtonReleased;
using antwika::time::fakes::FakeSleeper;
using antwika::tower_defence::BattleConfig;
using antwika::tower_defence::BattleScene;
using antwika::tower_defence::Campaign;
using antwika::tower_defence::CampaignConfig;
using antwika::tower_defence::CampaignSink;
using antwika::tower_defence::Cell;
using antwika::tower_defence::cellRect;
using antwika::tower_defence::layoutFor;
using antwika::tower_defence::LevelPlan;
using antwika::tower_defence::MobKind;
using antwika::tower_defence::RenderSink;
using antwika::tower_defence::ScoreOverlay;
using antwika::tower_defence::ScoreSink;
using antwika::tower_defence::Tile;
using antwika::tower_defence::TowerPlacementSink;
using antwika::tower_defence::Wave;
using antwika::tower_defence::WaveEntry;
using ::testing::NiceMock;
using ::testing::ReturnRef;

namespace
{
    constexpr Size kCanvas{.width = 960, .height = 720};
    constexpr std::uint32_t kWidth = 7;
    constexpr std::uint32_t kHeight = 5;

    // Small enough that the solver is not what these cases cost.
    CampaignConfig tinyCampaign()
    {
        return CampaignConfig{
            .seed = 3,
            .lives = 20,
            .levels = {LevelPlan{
                .level =
                    {.width = kWidth,
                     .height = kHeight,
                     .wallSpacing = 3},
                .battle = BattleConfig{},
                .waves = {Wave{
                    .entries = {WaveEntry{MobKind::Grunt, 2}},
                    .spawnPeriodTicks = 3,
                    .gapTicks = 0}}}}};
    }

    // The generated level decides where the road runs.
    // So a case needing open ground asks it rather than guessing.
    Cell anEmptyCell(const Campaign &campaign)
    {
        const auto &level = campaign.battle().level();
        for (std::uint32_t y = 0; y < level.height; ++y)
        {
            for (std::uint32_t x = 0; x < level.width; ++x)
            {
                const Cell cell{.x = x, .y = y};
                if (level.at(cell) == Tile::Empty)
                {
                    return cell;
                }
            }
        }
        return Cell{};
    }

    Translator english()
    {
        return Translator{antwika::i18n::kDefaultLocale};
    }

    TickEvent tick()
    {
        return TickEvent{
            .tick = 0,
            .event = Event{.name = antwika::engine::events::kTick}};
    }

    TickEvent other()
    {
        return TickEvent{
            .tick = 0, .event = Event{.name = "something.else"}};
    }

    TickEvent pressAt(
        const InputEventCodec &codec,
        const std::int32_t x,
        const std::int32_t y,
        const MouseButton button = MouseButton::Left)
    {
        return TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonPressed{
                .button = button, .position = {.x = x, .y = y}})};
    }

    TickEvent pressOn(const InputEventCodec &codec, const Cell &cell)
    {
        const auto layout = layoutFor(kCanvas, kWidth, kHeight);
        const auto rect = cellRect(*layout, cell);
        return pressAt(codec, rect.origin.x + 4, rect.origin.y + 4);
    }

    TEST(CampaignSinkTest, OnlyATickStepsTheCampaign)
    {
        Campaign campaign(tinyCampaign());
        CampaignSink sink(campaign);

        sink.handle(other());
        EXPECT_EQ(campaign.ticks(), 0U);

        sink.handle(tick());
        EXPECT_EQ(campaign.ticks(), 1U);
    }

    TEST(ScoreSinkTest, OnlyATickDescribesTheBar)
    {
        Campaign campaign(tinyCampaign());
        ScoreOverlay overlay(kCanvas);
        const Translator translator = english();
        ScoreSink sink(campaign, overlay, translator, 0);

        sink.handle(other());
        EXPECT_TRUE(overlay.commands().empty());

        sink.handle(tick());
        EXPECT_FALSE(overlay.commands().empty());
    }

    TEST(TowerPlacementSinkTest, ALeftPressOnOpenGroundBuilds)
    {
        Campaign campaign(tinyCampaign());
        const InputEventCodec codec;
        TowerPlacementSink sink(campaign, codec, kCanvas);

        const Cell open = anEmptyCell(campaign);
        sink.handle(pressOn(codec, open));

        ASSERT_EQ(campaign.battle().towers().size(), 1U);
        EXPECT_EQ(campaign.battle().towers()[0].cell, open);
    }

    TEST(TowerPlacementSinkTest, APressOnTheRunBuildsNothing)
    {
        Campaign campaign(tinyCampaign());
        const InputEventCodec codec;
        TowerPlacementSink sink(campaign, codec, kCanvas);

        sink.handle(pressOn(codec, campaign.battle().level().path[1]));
        EXPECT_TRUE(campaign.battle().towers().empty());
    }

    TEST(TowerPlacementSinkTest, APressOnTheScoreBarBuildsNothing)
    {
        Campaign campaign(tinyCampaign());
        const InputEventCodec codec;
        TowerPlacementSink sink(campaign, codec, kCanvas);

        sink.handle(pressAt(codec, 100, 4));
        EXPECT_TRUE(campaign.battle().towers().empty());
    }

    TEST(TowerPlacementSinkTest, OnlyALeftPressBuilds)
    {
        Campaign campaign(tinyCampaign());
        const InputEventCodec codec;
        TowerPlacementSink sink(campaign, codec, kCanvas);

        const auto layout = layoutFor(kCanvas, kWidth, kHeight);
        const auto rect = cellRect(*layout, anEmptyCell(campaign));

        sink.handle(pressAt(
            codec,
            rect.origin.x + 4,
            rect.origin.y + 4,
            MouseButton::Right));
        sink.handle(TickEvent{
            .tick = 0,
            .event = codec.encode(PointerButtonReleased{
                .button = MouseButton::Left,
                .position = {
                    .x = rect.origin.x + 4, .y = rect.origin.y + 4}})});
        sink.handle(tick());
        EXPECT_TRUE(campaign.battle().towers().empty());
    }

    TEST(TowerPlacementSinkTest, ACanvasWithNoRoomBuildsNothing)
    {
        Campaign campaign(tinyCampaign());
        const InputEventCodec codec;
        TowerPlacementSink sink(
            campaign, codec, {.width = 4, .height = 4});

        sink.handle(pressAt(codec, 1, 1));
        EXPECT_TRUE(campaign.battle().towers().empty());
    }

    TEST(RenderSinkTest, ATickDrawsAFrameAndPacesIt)
    {
        Campaign campaign(tinyCampaign());
        ScoreOverlay overlay(kCanvas);
        const BattleScene scene;
        FakeSleeper sleeper;
        NiceMock<MockRenderer> renderer;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(::testing::Return(true));
        ON_CALL(window, renderer()).WillByDefault(ReturnRef(renderer));

        EXPECT_CALL(renderer, present()).Times(1);

        RenderSink sink(
            window,
            scene,
            campaign,
            overlay,
            sleeper,
            std::chrono::milliseconds{5},
            kCanvas);
        sink.handle(tick());
        EXPECT_EQ(sleeper.requested().size(), 1U);
    }

    TEST(RenderSinkTest, AClosedWindowAndANonTickDrawNothing)
    {
        Campaign campaign(tinyCampaign());
        ScoreOverlay overlay(kCanvas);
        const BattleScene scene;
        FakeSleeper sleeper;
        NiceMock<MockWindow> window;
        ON_CALL(window, isOpen()).WillByDefault(::testing::Return(false));

        EXPECT_CALL(window, renderer()).Times(0);

        RenderSink sink(
            window,
            scene,
            campaign,
            overlay,
            sleeper,
            std::chrono::milliseconds{5},
            kCanvas);
        sink.handle(other());
        sink.handle(tick());
        EXPECT_TRUE(sleeper.requested().empty());
    }
} // namespace
