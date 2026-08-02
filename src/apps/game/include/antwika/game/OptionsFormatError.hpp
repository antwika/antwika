#pragma once

#include <stdexcept>

namespace antwika::game
{

    /**
     * @brief Thrown when a key binding is not one this app can read.
     *
     * That covers an options document that is not JSON at all, one that
     * is not this format's, a version this build cannot reach the
     * current one from, a member of the wrong shape, an action or a key
     * no name goes by, and a document binding two actions to one key or
     * binding one to a key this application spends above the tick loop.
     *
     * **A game.bind_key payload fails as this too**, and deliberately:
     * the failure category is "a binding could not be read from where it
     * was written", and a file and an event are two places one is
     * written rather than two categories. A caller catching a bad
     * options file wants the same answer about a recording that names an
     * action this build has never heard of.
     *
     * Its own type rather than antwika::replay's ReplayFormatError or
     * this app's SaveFormatError, because a bad options file is neither
     * a bad replay nor a bad save, and the house rule is one exception
     * type per failure category.
     */
    class OptionsFormatError final : public std::runtime_error
    {
    public:
        using std::runtime_error::runtime_error;
    };

} // namespace antwika::game
