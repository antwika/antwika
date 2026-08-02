#pragma once

#include <span>
#include <string>
#include <string_view>

#include "antwika/i18n/Locale.hpp"
#include "antwika/i18n/Lookup.hpp"
#include "antwika/i18n/MessageSet.hpp"
#include "antwika/i18n/Translation.hpp"

namespace antwika::i18n
{

    /**
     * @brief One locale's worth of lookups over one module's ids.
     *
     * It holds a Locale and nothing else -- the catalogues it reads are
     * compiled in and static, so a translator is copyable, cheap and safe
     * to store next to whatever owns the current language setting.
     * The fallback is always the default locale's catalogue; see lookup()
     * in Lookup.hpp for the rule it follows.
     *
     * The template parameter is the calling module's message set, which
     * is what keeps this library from naming any application: a module
     * declares its own ids and its own catalogues and writes
     * `using Translator = i18n::Translator<Messages>;` beside them.
     *
     * ## How an application uses this
     *
     * **A translator is injected, never reached for.**
     * One is constructed in `main()` and threaded down by `const
     * Translator &` into whatever words something -- a scene, a
     * `describe...()` function, a sink -- exactly as `log::ILogger &`
     * and `time::IClock &` are threaded through this project.
     * There is deliberately no `defaultTranslator()`, no thread-local
     * and no singleton: a global would be the one piece of global state
     * in a codebase that has none, and it would let any line anywhere
     * quietly start depending on the current language.
     * It outlives what it is handed to, following the ownership rule
     * every other injected reference here follows.
     *
     * **Which strings go through it.**
     * Everything a person reads on a screen or a terminal.
     * Not a log line, not an exception message, not a JSON key, not an
     * enumerator's name in a save file, not a replay event name and not
     * a `ui::WidgetId` -- those are formats and diagnostics, and a
     * persisted name that changed to suit a caption would break the file
     * it is written into.
     * A message that quotes one of them takes it as a `{0}` argument, so
     * the sentence is translated and the diagnostic inside it is not.
     *
     * **The locale may not become simulation state.**
     * Text that is *measured* is reproduced: an `antwika::ui` layout is
     * a function of the strings declared into it, and a hit-test is a
     * function of that layout, so a run recorded under one language and
     * replayed under another would resolve the same click to a different
     * widget.
     * An application that lays out from translated text therefore fixes
     * its locale at kDefaultLocale in `main()` and reads one from
     * nowhere else -- no environment variable and no flag, since neither
     * is carried by a recording.
     * Changing the language is a source change there, exactly as the
     * configured window size is.
     *
     * **Text a simulation decides on is decided as an id.**
     * `companion::Pet` picks a `companion::Saying` and the scene words
     * it; `ui_demo::DemoState` holds a `ui_demo::MessageId` and the
     * scene words that.
     * Storing the words themselves would put the active locale inside
     * the state a replay reproduces, which is the same mistake read from
     * the other end.
     */
    template <MessageSet Messages>
    class Translator final
    {
    public:
        /**
         * @brief The ids this translator answers for.
         */
        using Id = typename Messages::Id;

        /**
         * @brief Make a translator for one locale.
         * @param locale The language to translate into.
         */
        explicit constexpr Translator(Locale locale) noexcept
            : activeLocale{locale}
        {
        }

        /**
         * @brief The language being translated into.
         * @return The locale.
         */
        [[nodiscard]] constexpr Locale locale() const noexcept
        {
            return activeLocale;
        }

        /**
         * @brief Switch to another language.
         * @param locale The language to translate into from now on.
         */
        constexpr void setLocale(Locale locale) noexcept
        {
            activeLocale = locale;
        }

        /**
         * @brief Resolve one id, reporting where the text came from.
         * @param id The id to resolve.
         * @return The text and its origin.
         */
        [[nodiscard]] Translation lookup(Id id) const
        {
            return i18n::lookup<Messages>(
                id,
                Messages::catalogueFor(activeLocale),
                Messages::catalogueFor(kDefaultLocale));
        }

        /**
         * @brief Resolve one id, keeping only the text.
         * @param id The id to resolve.
         * @return The text, which is never empty.
         */
        [[nodiscard]] std::string text(Id id) const
        {
            return lookup(id).text;
        }

        /**
         * @brief Resolve one id and substitute arguments into it.
         * @param id The id to resolve.
         * @param args The arguments for its `{0}`-style placeholders.
         * @return The substituted text and the pattern's origin.
         */
        [[nodiscard]] Translation format(
            Id id, std::span<const std::string_view> args) const
        {
            return i18n::format<Messages>(
                id,
                args,
                Messages::catalogueFor(activeLocale),
                Messages::catalogueFor(kDefaultLocale));
        }

        /**
         * @brief Resolve and substitute, keeping only the text.
         * @param id The id to resolve.
         * @param args The arguments for its `{0}`-style placeholders.
         * @return The substituted text, which is never empty.
         */
        [[nodiscard]] std::string formatted(
            Id id, std::span<const std::string_view> args) const
        {
            return format(id, args).text;
        }

    private:
        Locale activeLocale;
    };

} // namespace antwika::i18n
