#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <antwika/console/ConsolePicture.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/tower_defence/Campaign.hpp"
#include "antwika/tower_defence/HighScore.hpp"
#include "antwika/tower_defence/IScoreStore.hpp"
#include "antwika/tower_defence/Messages.hpp"
#include "antwika/tower_defence/ScoreOverlay.hpp"

namespace antwika::tower_defence
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;

    struct BattleSummary final
    {
        std::uint64_t score = 0;
        std::uint32_t lives = 0;
        std::uint64_t ticks = 0;
        std::size_t towers = 0;
        std::size_t pathLength = 0;

        std::size_t level = 0;

        std::size_t wavesReleased = 0;

        CampaignPhase phase = CampaignPhase::Fighting;

        HighScore previousBest;

        HighScore best;

        std::vector<std::string> console;
    };

    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(
            const Campaign &, const ScoreOverlay &)>;

    struct TowerDefenceWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        const IInputEventCodec &codec;

        const Translator &translator;

        Size canvas;

        CampaignConfig campaign = {};

        std::optional<std::reference_wrapper<IScoreStore>> scoreStore =
            std::nullopt;

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        TickSinkFactory extraSink = {};
    };

    BattleSummary bootstrap(const TowerDefenceWiring &config);

    [[nodiscard]] std::optional<std::reference_wrapper<IScoreStore>>
    storeIfLive(
        IScoreStore &store,
        const std::optional<std::string> &replayPath);

    [[nodiscard]] std::string summaryLine(const BattleSummary &summary);

}
