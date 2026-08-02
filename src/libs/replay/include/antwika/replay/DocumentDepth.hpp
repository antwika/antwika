#pragma once

#include <cstddef>

#include <nlohmann/json.hpp>

namespace antwika::replay
{

    /**
     * @brief How deep a persisted document may nest.
     *
     * Far above every format in this tree -- a record is two levels
     * and the deepest save a handful -- and far below the depth at
     * which anything recursive over a value would eat the stack.
     * The bound exists because nlohmann's parser is iterative but its
     * copies and its error messages are not: a crafted line nested a
     * hundred thousand levels deep parses fine and then kills the
     * process inside the very validator that was meant to refuse it.
     */
    inline constexpr std::size_t kMaxDocumentDepth = 16;

    /**
     * @brief Ask whether a parsed value nests past that bound.
     *
     * Iterative on purpose: this guard runs before anything recursive
     * is allowed near the document, so it may not recurse itself.
     *
     * @param document The parsed value.
     * @return True when some value sits deeper than kMaxDocumentDepth.
     */
    [[nodiscard]] bool nestsTooDeep(const nlohmann::json &document);

} // namespace antwika::replay
