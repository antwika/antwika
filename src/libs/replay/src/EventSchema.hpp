#pragma once

#include <nlohmann/json.hpp>

/**
 * @file
 * @brief The JSON Schema fragment describing a single tick event's
 * shape, nested as the "items" of the whole-replay document schema
 * (ReplayJson.cpp).
 */
namespace antwika::replay::detail
{

    /**
     * @brief Get the {type, required, properties} shape of an encoded
     * TickEvent, without a top-level "$schema"/"title".
     * @return A JSON Schema fragment, for nesting inside a larger
     * schema's "items"/"properties".
     *
     * "tick" carries a "minimum": 0 constraint, but no matching
     * maximum -- json-schema-validator itself mishandles "minimum" for
     * integers above INT64_MAX, so ticks past that (within the full
     * uint64_t range antwika::time::Tick allows) fail to validate.
     */
    const nlohmann::json &tickEventShape();

} // namespace antwika::replay::detail
