#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace antwika::ecs_commons
{

    /**
     * @brief Max characters a Name holds, excluding the terminator.
     */
    inline constexpr std::size_t kNameMaxLength = 31;

    /**
     * @brief A short human-readable label attached to an entity.
     *
     * A fixed char buffer rather than a std::string, because
     * antwika::ecs::Component asks for trivial copyability and standard
     * layout and std::string has neither -- a component is copied between
     * the front and back buffers by a plain assignment, and a type with
     * an allocator behind it would put a heap operation inside the
     * commit.
     *
     * The buffer is not required to be null-terminated when the text
     * fills it exactly, which is why view() exists rather than callers
     * reaching for the array.
     */
    struct Name
    {
        std::array<char, kNameMaxLength> text{};

        /**
         * @brief Compare two names.
         * @param other The name to compare against.
         * @return True when both buffers hold the same bytes.
         */
        [[nodiscard]] bool operator==(const Name &other) const = default;
    };

    /**
     * @brief Build a Name from text, truncating anything too long.
     * @param text The label text to copy in.
     * @return A Name holding up to kNameMaxLength characters of text.
     *
     * Truncation rather than an exception: a label is a diagnostic, and
     * refusing to run a simulation over one that is too long to print
     * would let presentation decide whether the simulation happens.
     */
    [[nodiscard]] constexpr Name makeName(std::string_view text) noexcept
    {
        Name name{};
        const auto length =
            text.size() < kNameMaxLength ? text.size() : kNameMaxLength;

        for (std::size_t i = 0; i < length; ++i)
        {
            name.text[i] = text[i];
        }

        return name;
    }

    /**
     * @brief Read a Name back as text.
     * @param name The name to read.
     * @return A view of the characters before the first NUL, or the whole
     * buffer when it holds no NUL at all.
     *
     * The view borrows name's storage, so it is valid only for as long as
     * the Name it came from -- which, for a component read out of a
     * World, is until the next commit().
     */
    [[nodiscard]] constexpr std::string_view view(const Name &name) noexcept
    {
        std::size_t length = 0;
        while (length < kNameMaxLength && name.text[length] != '\0')
        {
            ++length;
        }

        return std::string_view(name.text.data(), length);
    }

} // namespace antwika::ecs_commons
