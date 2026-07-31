#pragma once

#include <stdexcept>

namespace antwika::game
{

    /**
     * @brief Thrown when a save document is not one this app can load.
     *
     * That covers text that is not JSON at all, a document that is not an
     * object, a "magic" that is not this format's, a schemaVersion this
     * build does not know how to reach, a missing or unexpected member,
     * and a field of the wrong shape.
     * A file that cannot be opened at all arrives here too, saying so in
     * its own words: a file that is not there is not a malformed one.
     *
     * Deliberately its own type rather than antwika::replay's
     * ReplayFormatError, because the two are different failure categories
     * and the house rule is one exception type per category.
     * A caller offering to load a save wants to catch a bad save without
     * also catching a bad replay.
     */
    class SaveFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::game
