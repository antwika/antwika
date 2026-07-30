#include "antwika/poker/PokerRoom.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <vector>

#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/SplitMix64Rng.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/replay/EngineLoop.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/PokerRoomSink.hpp"
#include "antwika/poker/PolicyAgent.hpp"
#include "antwika/poker/TablePrinter.hpp"

namespace antwika::poker
{

    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::Event;
    using antwika::event::EventDispatcher;
    using antwika::event::TickedEventDispatcher;
    using antwika::holdem::Deck;
    using antwika::holdem::IAgent;
    using antwika::holdem::makeSeatId;
    using antwika::holdem::SplitMix64Rng;
    using antwika::holdem::Table;
    using antwika::holdem::TableRunner;
    using antwika::log::Logger;
    using antwika::replay::EngineLoop;

    namespace
    {
        // Styles belong to seats rather than to players.
        // PolicyAgent holds no state beyond its style.
        // So a seat's style is the same as its occupant's own agent.
        constexpr std::array<AgentStyle, 3> kSeatStyles{
            AgentStyle::Balanced,
            AgentStyle::Tight,
            AgentStyle::Aggressive,
        };
    } // namespace

    PokerRoom::PokerRoom(IEngine &engine, IEventDispatcher &dispatcher)
        : engine(engine), dispatcher(dispatcher)
    {
    }

    void PokerRoom::run()
    {
        dispatcher.dispatch(
            Event{.name = "Running Antwika Poker"}); // GCOVR_EXCL_LINE
        engine.start();
    }

    RoomSummary bootstrap(
        IClock &clock,
        IAppender &appender,
        IFormatter &formatter,
        ILogPolicy &logPolicy,
        IEventSink &eventSink,
        IReplaySource &inputSource,
        std::ostream &out,
        RoomConfig config,
        std::optional<antwika::time::Tick> maxTicks,
        ITickEventSink *replayRecorder)
    {
        Logger logger(formatter, logPolicy, clock, appender);
        EventDispatcher dispatcher({eventSink});

        Table table(config.seatCount, config.blinds);
        BankrollLedger ledger;
        CashGame game(table, ledger, config.minimumBuyIn);

        std::vector<PolicyAgent> agents;
        agents.reserve(config.seatCount);
        std::vector<std::reference_wrapper<IAgent>> agentRefs;
        agentRefs.reserve(config.seatCount);
        for (std::size_t index = 0; index < config.seatCount; ++index)
        {
            agents.emplace_back(kSeatStyles[index % kSeatStyles.size()]);
        }
        for (auto &agent : agents)
        {
            agentRefs.emplace_back(agent);
        }

        SplitMix64Rng rng(config.seed);
        Deck deck(rng);
        TableRunner runner(table, deck, std::move(agentRefs));
        TablePrinter printer(out, game, table, clock, config.tableName);
        PokerRoomSink roomSink(runner, game, ledger, printer);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            roomSink, stopSignal};
        if (replayRecorder != nullptr)
        {
            timedSinks.push_back(*replayRecorder);
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        PokerRoom room(engine, tickedDispatcher);
        room.run();

        EngineLoop loop(engine, tickedDispatcher, inputSource);
        loop.run(stopSignal, maxTicks);

        static_cast<void>(game.cashOutEveryone());

        Chips leftOnTable = table.pot();
        for (std::size_t index = 0; index < table.seatCount(); ++index)
        {
            leftOnTable += table.seatAt(makeSeatId(index)).stack;
        }

        return RoomSummary{ // GCOVR_EXCL_LINE
            .handsPlayed = table.handsPlayed(),
            .balances = ledger.balances(),
            .chipsLeftOnTable = leftOnTable,
        };
    }

} // namespace antwika::poker
