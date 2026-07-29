#pragma once

#include <nlohmann/json.hpp>

/**
 * @file
 * @brief The JSON Schema fragment describing a single tick event's
 * shape, shared by the standalone event schema (EventJson.cpp) and the
 * whole-replay document schema (ReplayJson.cpp).
 */
namespace antwika::replay::detail
{

    /**
     * @brief Get the {type, required, properties} shape of an encoded
     * TickEvent, without a top-level "$schema"/"title".
     * @return A JSON Schema fragment, reusable standalone or nested
     * inside a larger schema's "items"/"properties".
     *
     * "tick" carries a "minimum": 0 constraint, but no matching
     * maximum -- json-schema-validator itself mishandles "minimum" for
     * integers above INT64_MAX, so ticks past that (not the full
     * uint64_t range BinaryEventCodec supports) fail to validate.
     */
    const nlohmann::json &timedEventShape();

} // namespace antwika::replay::detail
