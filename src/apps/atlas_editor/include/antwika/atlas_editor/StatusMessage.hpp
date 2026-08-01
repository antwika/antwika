#pragma once

#include <string>

#include <antwika/i18n/MessageId.hpp>

namespace antwika::atlas_editor
{

    using antwika::i18n::MessageId;

    /**
     * @brief The last thing worth telling the artist, before words.
     *
     * **An id and a datum rather than a sentence**, for the reason
     * every other piece of state here is what it is: a session's state
     * is regenerated from the recorded input on a replay, so a
     * translated string stored in it would put the active language
     * inside the thing a replay reproduces.
     * Keeping it an id leaves EditorUi the only place in this
     * application that has heard of a language, exactly as PetScene is
     * in apps/companion.
     *
     * The detail is a path or a failure's own words, and it is never
     * translated: it is a diagnostic, and a message that quotes one
     * takes it as a `{0}` argument so that the sentence around it is
     * worded and the diagnostic inside it is left alone.
     */
    struct StatusMessage final
    {
        /**
         * @brief Which message this is.
         */
        MessageId id{MessageId::AtlasLoaded};

        /**
         * @brief What it names, if it names anything.
         *
         * A path, or what a failed save or load reported.
         * Empty for a message with no `{0}` in it.
         */
        std::string detail;
    };

} // namespace antwika::atlas_editor
