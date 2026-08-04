#include "antwika/poker/PokerRoom.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <antwika/app/WindowCloseSource.hpp>
#include <antwika/engine/Engine.hpp>
#include <antwika/engine/StopSignal.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/event/EventDispatcher.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/event/TickedEventDispatcher.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/holdem/Deck.hpp>
#include <antwika/holdem/IAgent.hpp>
#include <antwika/holdem/SeatId.hpp>
#include <antwika/holdem/Table.hpp>
#include <antwika/holdem/TableRunner.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/simulation/EngineLoop.hpp>
#include <antwika/rng/SplitMix64Rng.hpp>

#include "antwika/poker/AgentStyle.hpp"
#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/PokerRoomSink.hpp"
#include "antwika/poker/PolicyAgent.hpp"
#include "antwika/poker/TablePrinter.hpp"
#include "antwika/poker/TableRenderSink.hpp"
#include "antwika/poker/TableScene.hpp"

namespace antwika::poker
{

    using antwika::app::WindowCloseSource;
    using antwika::engine::Engine;
    using antwika::engine::StopSignal;
    using antwika::event::Event;
    using antwika::event::EventDispatcher;
    using antwika::event::ITickEventSource;
    using antwika::event::TickedEventDispatcher;
    using antwika::gfx::ITexture;
    using antwika::gfx::IWindow;
    using antwika::gfx::WindowDesc;
    using antwika::holdem::Deck;
    using antwika::holdem::IAgent;
    using antwika::holdem::makeSeatId;
    using antwika::holdem::Table;
    using antwika::holdem::TableRunner;
    using antwika::log::Level;
    using antwika::rng::SplitMix64Rng;
    using antwika::simulation::EngineLoop;

    namespace
    {
        // A window that vanished on the last tick would hide the end.
        // The sink paces each frame, so this waits rather than spins.
        void holdFinalFrame(
            WindowCloseSource &source,
            const TableRenderSink &sink,
            const IWindow &window)
        {
            while (window.isOpen())
            {
                source.pumpEvents();
                sink.render();
            }
        }
    } // namespace

    PokerRoom::PokerRoom(IEngine &engine, ILogger &logger)
        : engine(engine), logger(logger)
    {
    }

    void PokerRoom::run()
    {
        logger.log(Level::Info, "Running Antwika Poker");
        engine.start();
    }

    RoomSummary bootstrap(const RoomSetup &setup)
    {
        IClock &clock = setup.clock;
        ITickEventSource &inputSource = setup.inputSource;
        const RoomConfig &config = setup.room;

        ILogger &logger = setup.logger;
        EventDispatcher dispatcher({setup.eventSink});

        Table table(config.seatCount, config.blinds);
        BankrollLedger ledger;
        CashGame game(table, ledger, config.minimumBuyIn);

        std::vector<PolicyAgent> agents;
        agents.reserve(config.seatCount);
        std::vector<std::reference_wrapper<IAgent>> agentRefs;
        agentRefs.reserve(config.seatCount);
        for (std::size_t index = 0; index < config.seatCount; ++index)
        {
            agents.emplace_back(
                config.seatStyles[index % config.seatStyles.size()],
                config.handStrengths,
                config.thresholds);
        }
        for (auto &agent : agents)
        {
            agentRefs.emplace_back(agent);
        }

        SplitMix64Rng rng(config.seed);
        Deck deck(rng);
        TableRunner runner(table, deck, std::move(agentRefs));
        TablePrinter printer(
            setup.out, game, table, clock, config.tableName);
        PokerRoomSink roomSink(runner, game, ledger, printer);
        StopSignal stopSignal;

        std::vector<std::reference_wrapper<ITickEventSink>> timedSinks{
            roomSink, stopSignal};

        // Declared before the optionals that hold references to it.
        std::unique_ptr<IWindow> tableWindow;
        std::unique_ptr<ITexture> atlasTexture;
        const TableScene scene;
        std::optional<TableRenderSink> renderSink;
        std::optional<WindowCloseSource> windowSource;
        ITickEventSource *source = &inputSource;

        if (setup.window.has_value())
        {
            const WindowSetup &window = setup.window->get();

            // Excluded on the line gcov attributes it to.
            // It is the unwind cleanup for desc's own title string.
            const WindowDesc desc{
                .title = config.tableName + " -- Antwika Poker",
                .size = window.size}; // GCOVR_EXCL_LINE
            tableWindow = window.backend.createWindow(desc);

            // A texture belongs to the renderer that made it.
            // This is the only place that has one.
            if (window.atlas != nullptr)
            {
                atlasTexture =
                    tableWindow->renderer().createTexture(*window.atlas);
            }

            renderSink.emplace(
                *tableWindow,
                window.size,
                scene,
                table,
                game,
                window.sleeper,
                window.framePeriod,
                config.tableName,
                atlasTexture
                    ? std::optional<std::reference_wrapper<const ITexture>>(
                          *atlasTexture)
                    : std::nullopt);

            // After roomSink, which is what steps the table.
            // A frame drawn before that shows the previous tick.
            timedSinks.push_back(*renderSink);

            windowSource.emplace(
                inputSource, window.backend, *tableWindow);
            source = &*windowSource;
        }

        if (setup.replayRecorder.has_value())
        {
            timedSinks.push_back(setup.replayRecorder->get());
        }
        TickedEventDispatcher tickedDispatcher(dispatcher, timedSinks);

        Engine engine(logger, tickedDispatcher);
        PokerRoom room(engine, logger);
        room.run();

        EngineLoop loop(engine, tickedDispatcher, *source);
        loop.run(stopSignal, setup.maxTicks);

        if (tableWindow)
        {
            if (setup.window->get().holdFinalFrame)
            {
                holdFinalFrame(*windowSource, *renderSink, *tableWindow);
            }
            tableWindow->close();
        }

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

    void printSummary(std::ostream &out, const RoomSummary &summary)
    {
        out << "\n=== " << summary.handsPlayed << " hands played ===\n";
        for (const auto &[player, balance] : summary.balances)
        {
            out << "  " << player << ": " << balance << '\n';
        }

        // Chips nobody won are only mentioned when there are some.
        // A session that ran to the end of a hand has none.
        if (summary.chipsLeftOnTable > 0)
        {
            out << "  (" << summary.chipsLeftOnTable
                << " chips left in an unfinished hand)\n";
        }
    }

} // namespace antwika::poker
