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

    /**
     * @brief Drives the poker table from the same tick event stream that
     * carries this application's own events.
     *
     * One engine tick is one step of the table: a deal, or one player
     * being asked what they want to do. That is what makes a game of
     * poker a replayable simulation rather than a loop of its own -- the
     * only thing a recorded session has to store is who joined with what
     * and when, because every card and every decision after that follows
     * from those deterministically.
     */
    class PokerRoomSink final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the sink over its collaborators.
         * @param runner Stepped once per engine tick.
         * @param game Where buy-in and cash-out events are applied, and
         * where busted players are sent home after each hand.
         * @param ledger Where deposit events are applied.
         * @param printer Narrates every step.
         */
        PokerRoomSink(
            TableRunner &runner,
            CashGame &game,
            BankrollLedger &ledger,
            TablePrinter &printer);

        /**
         * @brief Apply a tick event's effect.
         * @param event An engine tick advances the table by one step;
         * a deposit, buy-in or cash-out event moves money.
         * @throws BankrollError If a buy-in exceeds the player's
         * bankroll.
         * @throws CashGameError If a buy-in or cash-out cannot be
         * honoured -- under the minimum, no free seat, not seated, or
         * mid-hand.
         * @throws PokerEventError If a payload is not valid JSON or does
         * not match the event's schema.
         */
        void handle(const TickEvent &event) override;

    private:
        TableRunner &runner;
        CashGame &game;
        BankrollLedger &ledger;
        TablePrinter &printer;
    };

} // namespace antwika::poker
