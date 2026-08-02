#pragma once

#include <nlohmann/json.hpp>

/**
 * @file
 * @brief The JSON Schema describing a single tick event's shape, which
 * is one record of a replay (ReplayJson.cpp).
 *
 * It was the "items" of the whole-replay document schema before, and it
 * is a schema in its own right now that a record is a line in its own
 * right. The shape is the same one either way, which is exactly what
 * lets a version 1 document's array elements be read as records.
 */
namespace antwika::replay::detail
{

    /**
     * @brief Get the {type, required, properties} shape of an encoded
     * TickEvent, without a top-level "$schema"/"title".
     * @return A JSON Schema, usable on its own or nested inside a
     * larger schema's "items"/"properties".
     *
     * "tick" carries a "minimum": 0 constraint, but no matching
     * maximum -- json-schema-validator itself mishandles "minimum" for
     * integers above INT64_MAX, so ticks past that (within the full
     * uint64_t range antwika::time::Tick allows) fail to validate.
     */
    const nlohmann::json &tickEventShape();

} // namespace antwika::replay::detail
