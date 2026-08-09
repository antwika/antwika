#include "antwika/life/BoardSink.hpp"

#include <cstdint>
#include <limits>

#include <antwika/engine/Events.hpp>
#include <antwika/replay/JsonShapes.hpp>
#include <antwika/replay/PayloadJson.hpp>

#include "antwika/life/BoardSinkError.hpp"
#include "antwika/life/Cell.hpp"
#include "antwika/life/Events.hpp"

namespace antwika::life
{
    namespace
    {
        constexpr std::uint32_t kMaxCoordinate =
            std::numeric_limits<std::uint32_t>::max();

        nlohmann::json toggleCellSchema()
        {
            nlohmann::json schema = antwika::replay::documentShape(
                "life.toggle_cell payload", {"x", "y"});
            for (const char *field : {"x", "y"})
            {
                schema["properties"][field] =
                    antwika::replay::boundedCountShape(kMaxCoordinate);
            }
            return schema;
        } // GCOVR_EXCL_LINE
    }

    BoardSink::BoardSink(
        World &world, const Grid &grid, SystemScheduler &scheduler)
        : world(world), grid(grid), scheduler(scheduler)
    {
    }

    void BoardSink::handle(const TickEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            world.commit();
            scheduler.run(world, event.tick);
        }
        else if (event.event.name == events::kToggleCell)
        {
            const auto parsed =
                antwika::replay::parseAndValidatePayload<BoardSinkError>(
                    event.event.payload,
                    antwika::replay::validatorFor<toggleCellSchema>(),
                    "BoardSink: life.toggle_cell payload");
            const auto x = parsed.at("x").get<std::uint32_t>();
            const auto y = parsed.at("y").get<std::uint32_t>();

            if (!grid.contains(x, y))
            {
                throw BoardSinkError(
                    "BoardSink: life.toggle_cell names a cell outside "
                    "the board");
            }

            const auto entity = grid.entityAt(x, y);
            const auto wasAlive = world.get<Cell>(entity).alive;
            world.set<Cell>(entity, Cell{.alive = !wasAlive});
        }
    }

}
