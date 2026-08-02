#pragma once

#include <string_view>

#include <antwika/pattern/Controls.hpp>

namespace antwika::notation
{

    using antwika::pattern::Controls;

    /**
     * @brief Turns one word of a pattern string into what it carries.
     *
     * **The grammar knows what a word *is*; only this knows what one
     * *means*.**
     * That is what keeps this library as ignorant of music as
     * antwika::pattern is: `"0 3 5"` is three words, and whether they
     * are scale degrees, sample numbers or filter cutoffs is a decision
     * the application makes by supplying one of these.
     */
    class IWordReader
    {
    public:
        virtual ~IWordReader() = default;

        /**
         * @brief Read one word.
         * @param word The word, never empty.
         * @return What an event carrying it holds.
         * @throws NotationError If the word is not one this reader
         * accepts.
         */
        [[nodiscard]] virtual Controls read(
            std::string_view word) const = 0;
    };

} // namespace antwika::notation
