#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/holdem/TableRunner.hpp>

#include "antwika/poker/BankrollLedger.hpp"
#include "antwika/poker/CashGame.hpp"
#include "antwika/poker/TablePrinter.hpp"

namespace antwika::poker
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::holdem::TableRunner;

    class PokerRoomSink final : public ITickEventSink
    {
    public:
        PokerRoomSink(
            TableRunner &runner,
            CashGame &game,
            BankrollLedger &ledger,
            TablePrinter &printer);

        void handle(const TickEvent &event) override;

    private:
        TableRunner &runner;
        CashGame &game;
        BankrollLedger &ledger;
        TablePrinter &printer;
    };

}
