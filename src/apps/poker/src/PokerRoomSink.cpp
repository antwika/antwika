#include "antwika/poker/PokerRoomSink.hpp"

#include <string>

#include <nlohmann/json-schema.hpp>

#include <antwika/engine/Events.hpp>
#include <antwika/holdem/Chips.hpp>
#include <antwika/holdem/StepOutcome.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/poker/Events.hpp"
#include "antwika/poker/PokerEventError.hpp"

namespace antwika::poker
{

    using antwika::holdem::Chips;
    using antwika::holdem::StepKind;

    namespace
    {

        nlohmann::json playerAmountSchema(const char *title)
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = title;
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"player", "amount"}; // GCOVR_EXCL_LINE
            schema["properties"]["player"]["type"] = "string";
            schema["properties"]["player"]["minLength"] = 1;
            schema["properties"]["amount"]["type"] = "integer";
            schema["properties"]["amount"]["minimum"] = 1;
            return schema;
        }

        nlohmann::json playerSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "poker.cash_out payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"player"}; // GCOVR_EXCL_LINE
            schema["properties"]["player"]["type"] = "string";
            schema["properties"]["player"]["minLength"] = 1;
            return schema;
        }

        const nlohmann::json_schema::json_validator &depositValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                playerAmountSchema(
                    "poker.deposit payload")); // GCOVR_EXCL_LINE
            return validator;
        }

        const nlohmann::json_schema::json_validator &buyInValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                playerAmountSchema(
                    "poker.buy_in payload")); // GCOVR_EXCL_LINE
            return validator;
        }

        const nlohmann::json_schema::json_validator &cashOutValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                playerSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

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
                    depositValidator(),
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
                    buyInValidator(),
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
                    cashOutValidator(),
                    "PokerRoomSink: poker.cash_out payload");
            game.cashOut(parsed.at("player").get<std::string>());
        }
    }

} // namespace antwika::poker
