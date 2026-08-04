#include "antwika/poker/PokerRoomSink.hpp"

#include <string>

#include <antwika/engine/Events.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerEventError.hpp"

namespace antwika::poker
{

    using antwika::holdem::Chips;
    using antwika::holdem::StepKind;

    namespace
    {

        // A named player, which both payloads open with.
        // An empty name is nobody, and refused here.
        void describePlayer(nlohmann::json &schema)
        {
            schema["properties"]["player"] = antwika::replay::wordShape();
            schema["properties"]["player"]["minLength"] = 1;
        }

        nlohmann::json playerAmountSchema(const char *title)
        {
            nlohmann::json schema =
                antwika::replay::documentShape(title, {"player", "amount"});
            describePlayer(schema);
            schema["properties"]["amount"]["type"] = "integer";
            schema["properties"]["amount"]["minimum"] = 1;
            return schema;
        } // GCOVR_EXCL_LINE

        nlohmann::json depositSchema()
        {
            return playerAmountSchema("poker.deposit payload");
        }

        nlohmann::json buyInSchema()
        {
            return playerAmountSchema("poker.buy_in payload");
        }

        nlohmann::json cashOutSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "poker.cash_out payload", {"player"});
            describePlayer(schema);
            return schema;
        } // GCOVR_EXCL_LINE

    } // namespace

    PokerRoomSink::PokerRoomSink(
        TableRunner &runner,
        CashGame &game,
        BankrollLedger &ledger,
        TablePrinter &printer)
        : runner(runner), game(game), ledger(ledger), printer(printer)
    {
    }

    void PokerRoomSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            const auto outcome = runner.step();
            printer.printStep(outcome);

            // A stack that ran out is only busted between hands.
            // While a hand is live those chips still contest a pot.
            if (outcome.kind == StepKind::HandCompleted)
            {
                static_cast<void>(game.cashOutBustedPlayers());
            }
            return;
        }

        if (event.event.name == events::kDeposit)
        {
            const auto parsed =
                antwika::replay::parseAndValidatePayload<PokerEventError>(
                    event.event.payload,
                    antwika::replay::validatorFor<depositSchema>(),
                    "PokerRoomSink: poker.deposit payload");
            ledger.deposit(
                parsed.at("player").get<std::string>(),
                parsed.at("amount").get<Chips>());
            return;
        }

        if (event.event.name == events::kBuyIn)
        {
            const auto parsed =
                antwika::replay::parseAndValidatePayload<PokerEventError>(
                    event.event.payload,
                    antwika::replay::validatorFor<buyInSchema>(),
                    "PokerRoomSink: poker.buy_in payload");
            static_cast<void>(game.buyIn(
                parsed.at("player").get<std::string>(),
                parsed.at("amount").get<Chips>()));
            return;
        }

        if (event.event.name == events::kCashOut)
        {
            const auto parsed =
                antwika::replay::parseAndValidatePayload<PokerEventError>(
                    event.event.payload,
                    antwika::replay::validatorFor<cashOutSchema>(),
                    "PokerRoomSink: poker.cash_out payload");
            game.cashOut(parsed.at("player").get<std::string>());
        }
    }

} // namespace antwika::poker
