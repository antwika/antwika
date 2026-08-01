#pragma once

#include <cstdint>

#include <nlohmann/json.hpp>

/**
 * @file
 * @brief The JSON Schema fragments every persisted format in this code
 * base was writing its own copy of.
 *
 * A schema here is built as an nlohmann::json rather than written out as
 * a string, so a constraint is an ordinary C++ expression -- what a
 * std::int32_t holds, how many zoom levels there are -- and cannot drift
 * from the type the document decodes into.
 *
 * These four are the fragments the replay document, the game save and
 * the companion save each had a copy of. A shape only one format has
 * stays in that format, next to the rule it states.
 */
namespace antwika::replay
{

    /**
     * @brief The shape of a count: a whole number, never negative.
     * @return The schema fragment.
     *
     * Deliberately unbounded above, for the numbers a format is happy to
     * read whatever a file says: a tick count, a seed, or an index into
     * an array whose length the reader checks itself.
     */
    [[nodiscard]] nlohmann::json countShape();

    /**
     * @brief The shape of a count with a largest legal value.
     * @param maximum The largest value a document may state.
     * @return The schema fragment.
     *
     * The bound is what stops nlohmann narrowing in silence: `get<T>()`
     * takes the low bytes of anything wider without a word, so a schema
     * admitting a number the decode cannot hold reads a different number
     * back. A format that knows a value is narrower still than the C++
     * type it decodes into says so here, rather than checking it by hand
     * afterwards.
     */
    [[nodiscard]] nlohmann::json boundedCountShape(std::int64_t maximum);

    /**
     * @brief The shape of a signed coordinate.
     * @return The schema fragment, bounded by what a std::int32_t holds.
     */
    [[nodiscard]] nlohmann::json coordinateShape();

    /**
     * @brief The shape of a word: any string.
     * @return The schema fragment.
     *
     * Deliberately not a schema enum, even where the set of legal words
     * is closed: an unknown one is refused by the decode instead, so the
     * message can hold the name it did not know.
     */
    [[nodiscard]] nlohmann::json wordShape();

} // namespace antwika::replay
