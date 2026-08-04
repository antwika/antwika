#pragma once

#include <cstdint>
#include <initializer_list>
#include <string_view>

#include <nlohmann/json-schema.hpp>
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
 * These are the fragments the replay document, the game save, every
 * dump state and every event payload each had a copy of, plus the one
 * validator every one of them then wrapped its schema in. A shape only
 * one format has stays in that format, next to the rule it states.
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

    /**
     * @brief The members a document must state.
     * @param members The member names, in the order they read best.
     * @return The array a schema's "required" holds.
     *
     * A brace-init list assigned straight into a schema leaves an unwind
     * edge per element that no input can take, which is why every format
     * that wrote one had an exclusion comment beside it. Stated here
     * once, that comment is in one place.
     */
    [[nodiscard]] nlohmann::json requiredShape(
        std::initializer_list<std::string_view> members);

    /**
     * @brief The shape of an object with a closed set of members.
     * @param required The members a document must state.
     * @return The schema fragment.
     *
     * Closed rather than open: an unknown member is a document this
     * build cannot read, and reading it as the members it does know is
     * how a half-understood file gets written back out whole.
     */
    [[nodiscard]] nlohmann::json objectShape(
        std::initializer_list<std::string_view> required);

    /**
     * @brief The shape of a whole document, rather than a member of one.
     * @param title What the document is, for a validator's message.
     * @param required The members a document must state.
     * @return The schema fragment, an objectShape() that says which
     * dialect it is written in and what it describes.
     */
    [[nodiscard]] nlohmann::json documentShape(
        std::string_view title,
        std::initializer_list<std::string_view> required);

    /**
     * @brief The one validator a schema is read through.
     * @tparam BuildSchema The function that states the schema.
     * @return The validator, built once and shared from then on.
     *
     * Compiling a schema is worth doing once, so each format wrapped its
     * own in a static local -- twenty copies of four lines, and of the
     * reason the static guard's concurrency arms are excluded. The
     * schema is a template argument rather than a parameter because that
     * is what gives each format a static of its own: two formats passing
     * one function pointer would otherwise share one validator.
     */
    template <auto BuildSchema>
    [[nodiscard]] const nlohmann::json_schema::json_validator &
    validatorFor()
    {
        // The excluded lines carry the static guard.
        // Its concurrency arms are unreachable one-threaded.
        // The marker covers every instantiation, deliberately.
        // Each one says the same thing about its own guard.
        // See docs/confirming-unreachable-branches.md.
        // GCOVR_EXCL_START
        static const nlohmann::json_schema::json_validator validator(
            BuildSchema());
        // GCOVR_EXCL_STOP
        return validator;
    }

} // namespace antwika::replay
