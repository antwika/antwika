#include "antwika/game/GameStateReducer.hpp"

#include <antwika/engine/Events.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducerError.hpp"

namespace antwika::game
{

    namespace
    {
        nlohmann::json scoreIncrementSchema()
        {
            nlohmann::json schema = replay::documentShape(
                "game.score_increment payload", {"amount"});
            schema["properties"]["amount"] = replay::countShape();
            return schema;
        }

        std::uint64_t parseAmount(const std::string &payload)
        {
            const auto parsed = antwika::replay::parseAndValidatePayload<
                GameStateReducerError>(
                payload,
                replay::validatorFor<scoreIncrementSchema>(),
                "GameStateReducer: game.score_increment payload");
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
