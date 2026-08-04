#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

/**
 * @file
 * @brief The enumerator-to-persisted-name table every format in this
 * code base was writing its own copy of.
 *
 * A document holds a name rather than a number, so that reordering an
 * enumeration is not a change of format, and so that a file stays worth
 * reading by eye. Ten formats had grown the same three pieces to say
 * that: an array of names indexed by the enumerator, a lookup from the
 * enumerator to its name, and a scan back the other way.
 *
 * The table is a value rather than a base class, and both directions are
 * constexpr, so a format states its names once and the two lookups come
 * with them.
 */
namespace antwika::replay
{

    /**
     * @brief The two-way map between an enumeration and the names a
     * document holds for it.
     *
     * An aggregate, so a format writes its table as a constant beside the
     * enumeration it names:
     *
     * ```cpp
     * constexpr replay::NameTable<Tool, 3> kTools{
     *     {"paint", "erase", "fill"}};
     * ```
     *
     * @tparam Enum The enumeration, whose enumerators number up from zero
     * with no gaps -- which is what makes an index a value and back.
     * @tparam Count How many names there are, one per enumerator.
     */
    template <typename Enum, std::size_t Count>
    struct NameTable final
    {
        /**
         * @brief The names, addressed by the enumerator's own value.
         *
         * Persisted, so an entry may not change once written.
         */
        std::array<std::string_view, Count> names;

        /**
         * @brief Get the name a document holds for a value.
         * @param value The value to name.
         * @return Its name.
         */
        [[nodiscard]] constexpr std::string_view name(
            const Enum value) const noexcept
        {
            // Total rather than undefined for a value off the end.
            // Reading past the array is not how to report one.
            return names[static_cast<std::size_t>(value) % Count];
        }

        /**
         * @brief Get the value a persisted name refers to.
         * @param wanted The name to look up, as name() produced it.
         * @return The value, or nothing for a name nothing goes by.
         *
         * Deliberately an optional rather than a throw: which failure
         * category an unknown name belongs to is the format's own, and
         * the message that names it is written where it is thrown.
         */
        [[nodiscard]] constexpr std::optional<Enum> from(
            const std::string_view wanted) const noexcept
        {
            for (std::size_t index = 0; index < Count; ++index)
            {
                if (names[index] == wanted)
                {
                    return static_cast<Enum>(index);
                }
            }

            return std::nullopt;
        }
    };

} // namespace antwika::replay
