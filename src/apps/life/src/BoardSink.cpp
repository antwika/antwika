#include "antwika/life/BoardSink.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>

#include <antwika/engine/Events.hpp>

#include "antwika/life/Cell.hpp"
#include "antwika/life/Events.hpp"

namespace antwika::life
{
    namespace
    {
        std::uint32_t parseUInt32(std::string_view text)
        {
            std::uint32_t value{};
            std::from_chars(text.data(), text.data() + text.size(), value);
            return value;
        }
    } // namespace

    BoardSink::BoardSink(
        World &world, const Grid &grid, SystemScheduler &scheduler)
        : world(world), grid(grid), scheduler(scheduler)
    {
    }

    void BoardSink::handle(const TimedEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            world.commit();
            scheduler.run(world, event.tick);
        }
        else if (event.event.name == events::kToggleCell)
        {
            const std::string_view payload = event.event.payload;
            const auto separator = payload.find(',');
            const auto x = parseUInt32(payload.substr(0, separator));
            const auto y = parseUInt32(payload.substr(separator + 1));

            const auto entity = grid.entityAt(x, y);
            const auto wasAlive = world.get<Cell>(entity).alive;
            world.set<Cell>(entity, Cell{.alive = !wasAlive});
        }
    }

} // namespace antwika::life
