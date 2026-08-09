#pragma once

#include <chrono>
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
#include <antwika/time/ISleeper.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/companion/IPetStore.hpp"
#include "antwika/companion/Lineage.hpp"
#include "antwika/companion/Pet.hpp"

namespace antwika::companion
{

    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::gfx::Size;
    using antwika::input::IInputEventCodec;
    using antwika::log::ILogger;
    using antwika::event::ITickEventSource;
    using antwika::time::ISleeper;

    inline constexpr std::chrono::milliseconds kTickInterval{
        1000 / kTicksPerSecond};

    struct CompanionSummary final
    {
        antwika::time::Tick ticks = 0;
        std::uint32_t day = 0;
        std::uint32_t hunger = 0;
        std::uint32_t fun = 0;
        std::uint32_t happiness = 0;
        std::uint32_t energy = 0;
        std::uint32_t energyCeiling = 0;
        std::uint32_t meals = 0;
        std::uint32_t plays = 0;
        std::uint32_t disturbances = 0;
        std::uint32_t pesters = 0;
        std::uint32_t collapses = 0;
        std::uint32_t generation = 1;
        antwika::time::Tick bestTicks = 0;
        bool perished = false;

        std::vector<std::string> console;
    };

    using TickSinkFactory = std::function<
        std::unique_ptr<ITickEventSink>(const Pet &, const Lineage &)>;

    struct CompanionWiring final
    {
        ILogger &logger;

        IEventSink &eventSink;

        ITickEventSource &inputSource;

        const IInputEventCodec &codec;

        ISleeper &sleeper;

        PetConfig pet = {};

        Size canvas = {};

        std::optional<std::reference_wrapper<IPetStore>> store =
            std::nullopt;

        std::optional<
            std::reference_wrapper<antwika::console::ConsolePicture>>
            consoleOverlay = std::nullopt;

        bool consoleLoadEnabled = true;

        std::string stateDumpPath = "dump_state.json";

        std::chrono::milliseconds tickInterval = kTickInterval;

        std::optional<antwika::time::Tick> maxTicks = std::nullopt;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder = std::nullopt;

        TickSinkFactory extraSink = {};
    };

    CompanionSummary bootstrap(const CompanionWiring &config);

    [[nodiscard]] std::optional<std::reference_wrapper<IPetStore>>
        storeIfLive(
            IPetStore &store,
            const std::optional<std::string> &replayPath);

    void announceHowToStop(ILogger &logger, bool drawsNothing);

    [[nodiscard]] std::string summaryLine(const CompanionSummary &summary);

}
