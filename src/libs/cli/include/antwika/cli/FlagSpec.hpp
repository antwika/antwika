#pragma once

#include <string_view>

namespace antwika::cli
{

    /**
     * @brief One flag a program accepts, and everything the parser and
     * the help text need to know about it.
     *
     * One table is what keeps the two in step: a flag that parses but is
     * not documented, or documented but not accepted, is not expressible
     * from here.
     */
    struct FlagSpec
    {
        /**
         * @brief The flag as it is typed, leading dashes and all.
         */
        std::string_view name{};

        /**
         * @brief What the flag's value is called in the help text.
         *
         * Empty means the flag takes no value at all, and giving it one
         * is an unexpected argument rather than its value.
         */
        std::string_view valueName{};

        /**
         * @brief One line saying what the flag does.
         */
        std::string_view help{};
    };

} // namespace antwika::cli
