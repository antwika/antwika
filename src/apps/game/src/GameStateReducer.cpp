#include "antwika/game/GameStateReducer.hpp"

#include <charconv>
#include <string_view>
#include <system_error>

#include <antwika/engine/Events.hpp>

#include "antwika/game/Events.hpp"
#include "antwika/game/GameStateReducerError.hpp"

namespace antwika::game
{

    namespace
    {
        std::uint64_t parseUInt64(std::string_view text)
        {
            std::uint64_t value{};
            const auto result = std::from_chars(
                text.data(), text.data() + text.size(), value);
            if (result.ec != std::errc{} ||
                result.ptr != text.data() + text.size())
            {
                throw GameStateReducerError(
                    "GameStateReducer: game.score_increment payload is "
                    "not a plain, in-range base-10 unsigned integer");
            }
            return value;
        }
    } // namespace

    GameStateReducer::GameStateReducer(GameState &state) : state(state)
    {
    }

    void GameStateReducer::handle(const TimedEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            ++state.ticksProcessed;
        }
        else if (event.event.name == events::kScoreIncrement)
        {
            state.score += parseUInt64(event.event.payload);
        }
    }

} // namespace antwika::game
