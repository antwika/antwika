#include "antwika/game/GameStateReducer.hpp"

#include <nlohmann/json-schema.hpp>

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducerError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json scoreIncrementSchema()
        {
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "game.score_increment payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"amount"}; // GCOVR_EXCL_LINE
            schema["properties"]["amount"]["type"] = "integer";
            schema["properties"]["amount"]["minimum"] = 0;
            return schema;
        }

        const nlohmann::json_schema::json_validator &
        scoreIncrementValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                scoreIncrementSchema()); // GCOVR_EXCL_LINE
            return validator;
        }

        std::uint64_t parseAmount(const std::string &payload)
        {
            nlohmann::json parsed;
            try
            {
                parsed = nlohmann::json::parse(payload);
            }
            catch (const nlohmann::json::parse_error &) // GCOVR_EXCL_LINE
            {
                throw GameStateReducerError(
                    "GameStateReducer: game.score_increment payload is "
                    "not valid JSON");
            }

            try
            {
                scoreIncrementValidator().validate(parsed);
            }
            catch (const std::exception &error) // GCOVR_EXCL_LINE
            {
                throw GameStateReducerError(
                    std::string(
                        "GameStateReducer: game.score_increment payload "
                        "failed schema validation: ") +
                    error.what());
            }
            return parsed.at("amount").get<std::uint64_t>();
        }
    } // namespace

    GameStateReducer::GameStateReducer(GameState &state) : state(state)
    {
    }

    void GameStateReducer::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            ++state.ticksProcessed;
        }
        else if (event.event.name == events::kScoreIncrement)
        {
            state.score += parseAmount(event.event.payload);
        }
    }

} // namespace antwika::game
