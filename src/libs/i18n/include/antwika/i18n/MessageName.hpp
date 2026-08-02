#pragma once

#include <string_view>

namespace antwika::i18n
{

    /**
     * @brief One id and the enumerator name it was declared under.
     *
     * A module's table of these is the list of everything there is: it
     * is what a catalogue is checked for completeness *against*, and it
     * is where a total lookup gets the text for an id no catalogue
     * knows.
     * The two were once an array and a switch that had to agree with
     * each other; one table of pairs cannot disagree with itself.
     */
    template <typename Id>
    struct MessageName final
    {
        /**
         * @brief The id being named.
         */
        Id id{};

        /**
         * @brief The enumerator's own name, as written in the source.
         */
        std::string_view name;
    };

} // namespace antwika::i18n
