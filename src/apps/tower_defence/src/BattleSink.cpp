#include "antwika/tower_defence/BattleSink.hpp"

#include <antwika/engine/Events.hpp>

namespace antwika::tower_defence
{

    BattleSink::BattleSink(Battle &battle) : battle(battle)
    {
    }

    void BattleSink::handle(const TickEvent &event)
    {
        if (event.event.name != antwika::engine::events::kTick)
        {
            return;
        }

        battle.step();
    }

} // namespace antwika::tower_defence
