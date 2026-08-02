#pragma once

#include <optional>
#include <string>

#include "antwika/ui_demo/MessageId.hpp"

namespace antwika::ui_demo
{

    /**
     * @brief What the demo last said about what happened, before words.
     *
     * **An id rather than a sentence**, because DemoState is what a
     * replay regenerates from the recorded clicks and key presses: a
     * translated string kept there would put the active language inside
     * the thing a replay reproduces, which is the rule
     * antwika/i18n/Translator.hpp states at length.
     * DemoScene is therefore the only place in this application that has
     * heard of a language, exactly as PetScene is in apps/companion.
     *
     * The argument is one of two things and never both.
     * A `datum` is something the tick path produced -- a number, or the
     * characters somebody typed -- and is shown as it stands.
     * An `argId` is a second message, for the case where the argument is
     * itself something the catalogue words, which is what "showing
     * {page}" needs.
     */
    struct DemoMessage final
    {
        /**
         * @brief Which message this is.
         */
        MessageId id{MessageId::Cancelled};

        /**
         * @brief The `{0}` argument as a plain datum, if it is one.
         */
        std::string datum;

        /**
         * @brief The `{0}` argument as another message, if it is one.
         *
         * Takes precedence over `datum`, which is empty whenever this
         * has a value.
         */
        std::optional<MessageId> argId;
    };

} // namespace antwika::ui_demo
