#include "antwika/poker/StateDump.hpp"

#include <array>
#include <cstddef>
#include <exception>

#include <nlohmann/json-schema.hpp>

#include <antwika/holdem/Card.hpp>
#include <antwika/holdem/Stage.hpp>

namespace antwika::poker
{

    using antwika::holdem::Card;
    using antwika::holdem::kCardCount;
    using antwika::holdem::kHoleCardCount;
    using antwika::holdem::makeSeatId;
    using antwika::holdem::rawValue;

    namespace
    {
        // The names a dump document holds, one per stage.
        // Persisted, so they may not change once written.
        constexpr std::array<std::string_view, 5> kStageNames{
            "pre_flop", "flop", "turn", "river", "showdown"};

        [[nodiscard]] std::string_view stageName(Stage stage) noexcept
        {
            return kStageNames
                [static_cast<std::size_t>(stage) % kStageNames.size()];
        }

        [[nodiscard]] Stage stageNamed(const std::string &name)
        {
            for (std::size_t index = 0; index < kStageNames.size();
                 ++index)
            {
                if (kStageNames[index] == name)
                {
                    return static_cast<Stage>(index);
                }
            }

            throw StateDumpError(
                "antwika::poker: dump names a stage this build does "
                "not know: "
                + name);
        }

        [[nodiscard]] nlohmann::json cardsToJson(const auto &cards)
        {
            auto encoded = nlohmann::json::array();

            for (const auto card : cards)
            {
                encoded.push_back(rawValue(card));
            }

            return encoded;

            // gcov puts the returned value's unwind block here.
            // No input reaches it.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] Card cardFrom(const nlohmann::json &value)
        {
            const auto raw = value.get<std::uint32_t>();

            if (raw >= kCardCount)
            {
                throw StateDumpError(
                    "antwika::poker: dump names a card past the "
                    "deck: "
                    + std::to_string(raw));
            }

            return static_cast<Card>(raw);
        }

        [[nodiscard]] nlohmann::json seatToJson(
            const antwika::holdem::Seat &seat)
        {
            nlohmann::json encoded;

            encoded["stack"] = seat.stack;
            encoded["committed"] = seat.committed;
            encoded["roundCommitted"] = seat.roundCommitted;
            encoded["occupied"] = seat.occupied;
            encoded["inHand"] = seat.inHand;
            encoded["acted"] = seat.actedThisRound;
            encoded["mayRaise"] = seat.mayRaise;
            encoded["hole"] = cardsToJson(seat.holeCards);

            return encoded;

            // The returned value's unwind block, as above.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] antwika::holdem::Seat seatFrom(
            const nlohmann::json &encoded)
        {
            antwika::holdem::Seat seat;

            seat.stack = encoded.at("stack").get<Chips>();
            seat.committed = encoded.at("committed").get<Chips>();
            seat.roundCommitted =
                encoded.at("roundCommitted").get<Chips>();
            seat.occupied = encoded.at("occupied").get<bool>();
            seat.inHand = encoded.at("inHand").get<bool>();
            seat.actedThisRound = encoded.at("acted").get<bool>();
            seat.mayRaise = encoded.at("mayRaise").get<bool>();

            const auto &hole = encoded.at("hole");

            if (hole.size() != kHoleCardCount)
            {
                throw StateDumpError(
                    "antwika::poker: a seat holds exactly two hole "
                    "cards");
            }

            for (std::size_t index = 0; index < kHoleCardCount; ++index)
            {
                seat.holeCards[index] = cardFrom(hole.at(index));
            }

            return seat;
        }

        [[nodiscard]] nlohmann::json resultToJson(
            const antwika::holdem::HandResult &result)
        {
            nlohmann::json encoded;

            encoded["pot"] = result.pot;
            encoded["stage"] = std::string(stageName(result.stage));
            encoded["board"] = cardsToJson(result.board);
            encoded["payouts"] = nlohmann::json::array();

            for (const auto &payout : result.payouts)
            {
                encoded["payouts"].push_back(
                    {{"seat", rawValue(payout.seat)},
                     {"amount", payout.amount}});
            }

            encoded["showdown"] = nlohmann::json::array();

            for (const auto &entry : result.showdown)
            {
                encoded["showdown"].push_back(
                    {{"seat", rawValue(entry.seat)},
                     {"hole", cardsToJson(entry.holeCards)},
                     {"value",
                      static_cast<std::uint32_t>(entry.value)}});
            }

            return encoded;

            // The returned value's unwind block, as above.
        } // GCOVR_EXCL_LINE

        [[nodiscard]] antwika::holdem::HandResult resultFrom(
            const nlohmann::json &encoded)
        {
            antwika::holdem::HandResult result;

            result.pot = encoded.at("pot").get<Chips>();
            result.stage =
                stageNamed(encoded.at("stage").get<std::string>());

            for (const auto &card : encoded.at("board"))
            {
                result.board.push_back(cardFrom(card));
            }

            for (const auto &payout : encoded.at("payouts"))
            {
                result.payouts.push_back(
                    {.seat = makeSeatId(
                         payout.at("seat").get<std::size_t>()),
                     .amount = payout.at("amount").get<Chips>()});
            }

            for (const auto &entry : encoded.at("showdown"))
            {
                antwika::holdem::ShowdownEntry shown;

                shown.seat =
                    makeSeatId(entry.at("seat").get<std::size_t>());
                shown.value = static_cast<antwika::holdem::HandValue>(
                    entry.at("value").get<std::uint32_t>());

                const auto &hole = entry.at("hole");

                if (hole.size() != kHoleCardCount)
                {
                    throw StateDumpError(
                        "antwika::poker: a showdown shows exactly "
                        "two hole cards");
                }

                for (std::size_t index = 0; index < kHoleCardCount;
                     ++index)
                {
                    shown.holeCards[index] = cardFrom(hole.at(index));
                }

                result.showdown.push_back(shown);
            }

            return result;

            // The returned value's unwind block, as above.
        } // GCOVR_EXCL_LINE

        nlohmann::json stateSchema()
        {
            // Cards are bounded here, once.
            // A decode's cast is then total.
            // Everything structural is bounded by type.
            nlohmann::json card;
            card["type"] = "integer";
            card["minimum"] = 0;
            card["maximum"] = kCardCount - 1;

            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "antwika poker dump state";
            schema["type"] = "object";
            schema["required"] = {
                "bits",
                "deck",
                "table",
                "balances",
                "names",
                "printer"}; // GCOVR_EXCL_LINE
            schema["properties"]["bits"]["type"] = "integer";
            schema["properties"]["deck"]["type"] = "object";
            schema["properties"]["deck"]["required"] = {
                "cards", "dealt"}; // GCOVR_EXCL_LINE
            schema["properties"]["deck"]["properties"]["cards"]["type"] =
                "array";
            schema["properties"]["deck"]["properties"]["cards"]["items"] =
                card;
            schema["properties"]["deck"]["properties"]["cards"]
                  ["minItems"] = kCardCount;
            schema["properties"]["deck"]["properties"]["cards"]
                  ["maxItems"] = kCardCount;
            schema["properties"]["deck"]["properties"]["dealt"]
                  ["type"] = "integer";
            schema["properties"]["deck"]["properties"]["dealt"]
                  ["minimum"] = 0;
            schema["properties"]["deck"]["properties"]["dealt"]
                  ["maximum"] = kCardCount;
            schema["properties"]["table"]["type"] = "object";
            schema["properties"]["balances"]["type"] = "object";
            schema["properties"]["names"]["type"] = "array";
            schema["properties"]["names"]["items"]["type"] = "string";
            schema["properties"]["printer"]["type"] = "object";
            return schema;
        }

        const nlohmann::json_schema::json_validator &stateValidator()
        {
            // The excluded closing line carries the static guard.
            // Its concurrency arms are unreachable one-threaded.
            static const nlohmann::json_schema::json_validator validator(
                stateSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

    antwika::replay::MigrationChain standardStateDumpMigrations()
    {
        return antwika::replay::MigrationChain(
            {}, kStateDumpVersion); // GCOVR_EXCL_LINE
    }

    nlohmann::json roomDumpToJson(const RoomDump &dump)
    {
        nlohmann::json encoded;

        encoded["bits"] = dump.bits;
        encoded["deck"]["cards"] = cardsToJson(dump.deck.cards);
        encoded["deck"]["dealt"] = dump.deck.dealt;

        auto &table = encoded["table"];
        table["seats"] = nlohmann::json::array();

        for (const auto &seat : dump.table.seats)
        {
            table["seats"].push_back(seatToJson(seat));
        }

        if (dump.table.result.has_value())
        {
            table["result"] = resultToJson(*dump.table.result);
        }

        if (dump.table.toAct.has_value())
        {
            table["toAct"] = rawValue(*dump.table.toAct);
        }

        table["pot"] = dump.table.pot;
        table["currentBet"] = dump.table.betting.currentBet;
        table["lastRaiseSize"] = dump.table.betting.lastRaiseSize;
        table["stage"] = std::string(stageName(dump.table.stage));
        table["board"] = cardsToJson(dump.table.board);
        table["handCount"] = dump.table.handCount;
        table["button"] = rawValue(dump.table.button);
        table["handInProgress"] = dump.table.handInProgress;

        encoded["balances"] = dump.balances;
        encoded["names"] = dump.names;

        auto &printer = encoded["printer"];
        printer["notes"] = nlohmann::json::array();

        for (const auto &note : dump.printer.notes)
        {
            printer["notes"].push_back(
                {{"roundStake", note.roundStake},
                 {"foldedOn", std::string(stageName(note.foldedOn))},
                 {"dealtIn", note.dealtIn},
                 {"folded", note.folded}});
        }

        if (dump.printer.smallBlindSeat.has_value())
        {
            printer["smallBlind"] =
                rawValue(*dump.printer.smallBlindSeat);
        }

        if (dump.printer.bigBlindSeat.has_value())
        {
            printer["bigBlind"] = rawValue(*dump.printer.bigBlindSeat);
        }

        printer["stage"] = std::string(stageName(dump.printer.stage));
        printer["boardShown"] = dump.printer.boardShown;

        return encoded;

        // The returned value's unwind block, as above.
    } // GCOVR_EXCL_LINE

    RoomDump roomDumpFromJson(const nlohmann::json &state)
    {
        try
        {
            stateValidator().validate(state);
        }
        // The validator's failure type is the library's business.
        // What this format promises is StateDumpError.
        catch (const std::exception &failed) // GCOVR_EXCL_LINE
        {
            throw StateDumpError(
                std::string(
                    "antwika::poker: dump state failed schema "
                    "validation: ")
                + failed.what());
        }

        RoomDump dump;

        dump.bits = state.at("bits").get<std::uint64_t>();

        const auto &deck = state.at("deck");
        dump.deck.dealt = deck.at("dealt").get<std::size_t>();

        for (std::size_t index = 0; index < kCardCount; ++index)
        {
            dump.deck.cards[index] =
                cardFrom(deck.at("cards").at(index));
        }

        const auto &table = state.at("table");

        for (const auto &seat : table.at("seats"))
        {
            dump.table.seats.push_back(seatFrom(seat));
        }

        if (table.contains("result"))
        {
            dump.table.result = resultFrom(table.at("result"));
        }

        if (table.contains("toAct"))
        {
            dump.table.toAct =
                makeSeatId(table.at("toAct").get<std::size_t>());
        }

        dump.table.pot = table.at("pot").get<Chips>();
        dump.table.betting.currentBet =
            table.at("currentBet").get<Chips>();
        dump.table.betting.lastRaiseSize =
            table.at("lastRaiseSize").get<Chips>();
        dump.table.stage =
            stageNamed(table.at("stage").get<std::string>());

        for (const auto &card : table.at("board"))
        {
            dump.table.board.push_back(cardFrom(card));
        }

        dump.table.handCount =
            table.at("handCount").get<std::uint64_t>();
        dump.table.button =
            makeSeatId(table.at("button").get<std::size_t>());
        dump.table.handInProgress =
            table.at("handInProgress").get<bool>();

        dump.balances = state.at("balances")
                            .get<std::map<std::string, Chips>>();
        dump.names =
            state.at("names").get<std::vector<std::string>>();

        const auto &printer = state.at("printer");

        for (const auto &note : printer.at("notes"))
        {
            dump.printer.notes.push_back(PrinterNote{
                .roundStake = note.at("roundStake").get<Chips>(),
                .foldedOn = stageNamed(
                    note.at("foldedOn").get<std::string>()),
                .dealtIn = note.at("dealtIn").get<bool>(),
                .folded = note.at("folded").get<bool>()});
        }

        if (printer.contains("smallBlind"))
        {
            dump.printer.smallBlindSeat = makeSeatId(
                printer.at("smallBlind").get<std::size_t>());
        }

        if (printer.contains("bigBlind"))
        {
            dump.printer.bigBlindSeat =
                makeSeatId(printer.at("bigBlind").get<std::size_t>());
        }

        dump.printer.stage =
            stageNamed(printer.at("stage").get<std::string>());
        dump.printer.boardShown =
            printer.at("boardShown").get<std::size_t>();

        return dump;

        // The returned value's unwind block, as above.
    } // GCOVR_EXCL_LINE

} // namespace antwika::poker
