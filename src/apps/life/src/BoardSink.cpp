#include "antwika/life/BoardSink.hpp"

#include <cstdint>
#include <limits>

#include <nlohmann/json-schema.hpp>

#include <antwika/engine/Events.hpp>
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
            nlohmann::json schema;
            schema["$schema"] = "http://json-schema.org/draft-07/schema#";
            schema["title"] = "life.toggle_cell payload";
            schema["type"] = "object";
            schema["additionalProperties"] = false;
            schema["required"] = {"x", "y"}; // GCOVR_EXCL_LINE
            for (const char *field : {"x", "y"})
            {
                schema["properties"][field]["type"] = "integer";
                schema["properties"][field]["minimum"] = 0;
                schema["properties"][field]["maximum"] = kMaxCoordinate;
            }
            return schema;
        }

        const nlohmann::json_schema::json_validator &toggleCellValidator()
        {
            static const nlohmann::json_schema::json_validator validator(
                toggleCellSchema()); // GCOVR_EXCL_LINE
            return validator;
        }
    } // namespace

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
                    toggleCellValidator(),
                    "BoardSink: life.toggle_cell payload");
            const auto x = parsed.at("x").get<std::uint32_t>();
            const auto y = parsed.at("y").get<std::uint32_t>();

            const auto entity = grid.entityAt(x, y);
            const auto wasAlive = world.get<Cell>(entity).alive;
            world.set<Cell>(entity, Cell{.alive = !wasAlive});
        }
    }

} // namespace antwika::life
