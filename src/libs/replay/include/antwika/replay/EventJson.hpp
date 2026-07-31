#pragma once

#include <nlohmann/json.hpp>

#include <antwika/event/Event.hpp>
#include <antwika/event/TickEvent.hpp>

/**
 * @file
 * @brief JSON conversion for antwika::event::Event and TickEvent.
 *
 * to_json/from_json are declared inside antwika::event -- the type's own
 * namespace -- so nlohmann's argument-dependent lookup finds them for
 * any json j; j.get<Event>() or json(event), from any translation unit
 * that includes this header.
 */
namespace antwika::event
{

    /**
     * @brief Serialize an Event to {"name":..., "payload":...}.
     */
    void to_json(nlohmann::json &j, const Event &event);

    /**
     * @brief Deserialize an Event from {"name":..., "payload":...}.
     * @throws nlohmann::json::out_of_range If a required field is
     * missing.
     * @throws nlohmann::json::type_error If a field has the wrong type.
     */
    void from_json(const nlohmann::json &j, Event &event);

    /**
     * @brief Serialize a TickEvent to {"tick":..., "event":{...}}.
     */
    void to_json(nlohmann::json &j, const TickEvent &event);

    /**
     * @brief Deserialize a TickEvent from {"tick":..., "event":{...}}.
     * @throws nlohmann::json::out_of_range If a required field is
     * missing.
     * @throws nlohmann::json::type_error If a field has the wrong type.
     */
    void from_json(const nlohmann::json &j, TickEvent &event);

} // namespace antwika::event
