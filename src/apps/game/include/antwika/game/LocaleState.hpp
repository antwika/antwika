#pragma once

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/i18n/Messages.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/game/Messages.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::i18n::Locale;

    /**
     * @brief The translator the language names themselves come from.
     *
     * A second set rather than more ids here, because `Locale` is
     * i18n's own closed enum and the text for each of its values is
     * therefore i18n's to carry -- see i18n::MessageId. Reading them
     * through the *active* locale is the point: the picker says
     * "English"/"Swedish" while English is on and "Engelska"/"Svenska"
     * while Swedish is.
     */
    using LanguageTranslator =
        antwika::i18n::Translator<antwika::i18n::Messages>;

    /**
     * @brief The language the run is being played in, and the one
     * Translator every scene words itself with.
     *
     * **The locale is simulation state**, for exactly the reason the
     * mode and the camera are (see AppMode.hpp and blog/013): a layout
     * is a function of the strings declared into it and a hit-test is a
     * function of that layout, so the language decides what a click
     * *means*. i18n::Translator states the same rule from the library's
     * side and tells an application to fix its locale in main(). This
     * class is what it costs to lift that restriction honestly rather
     * than to break it: the locale moves *into* the tick path instead of
     * escaping it, so a replay reproduces the language the run was
     * played in the same way it reproduces the camera.
     *
     * **Nothing here is persisted that can be regenerated.** A player
     * changing the language mid-session does it by clicking the options
     * screen, and that click is already in the recording, so the change
     * is derived from it on replay rather than written down again --
     * which is the same reason rebinding a key defines no event. What
     * *is* announced, once, is the locale a live run starts in, because
     * that came from the player's options file and no amount of replayed
     * input implies it; LocaleSource does that, exactly as BindingSource
     * announces the machine's starting key bindings.
     *
     * **A change is staged and lands at the tick boundary**, the way
     * AppModeState's does and for the same reason. The sinks of one tick
     * run in a fixed order over each event, so a language that changed
     * part-way through one would let a single press be resolved against
     * two different layouts -- the button under the pointer when it went
     * down would not be the button under it when the press was read.
     * Staging makes that not expressible.
     *
     * It is an ITickEventSink so the boundary is a place rather than a
     * convention: **register it beside AppModeState, immediately after
     * InputFold**, ahead of every sink that describes a layout.
     */
    class LocaleState final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the state over the language a run starts in.
         * @param initial The locale both current and staged begin as.
         * The application fills this from the options file by way of
         * LocaleSource; a test whose subject is not the language leaves
         * it defaulted, the same way one leaves GameWiring::config.
         */
        explicit LocaleState(
            Locale initial = antwika::i18n::kDefaultLocale) noexcept
            : active(initial), languageNames(initial), staged(initial)
        {
        }

        LocaleState(const LocaleState &) = delete;
        LocaleState(LocaleState &&) = delete;

        LocaleState &operator=(const LocaleState &) = delete;
        LocaleState &operator=(LocaleState &&) = delete;

        /**
         * @brief Get the translator every scene words itself with.
         *
         * Handed out as a const reference, which is what keeps this the
         * only place a language can change: a scene holding it can read
         * a caption and cannot call setLocale().
         *
         * @return The translator, at the committed locale.
         */
        [[nodiscard]] const Translator &translator() const noexcept
        {
            return active;
        }

        /**
         * @brief Get the translator the language names come from.
         *
         * Kept at the same locale as translator(), so a picker built
         * from it names every language in the language now on.
         *
         * @return The translator, at the committed locale.
         */
        [[nodiscard]] const LanguageTranslator &languages() const noexcept
        {
            return languageNames;
        }

        /**
         * @brief Get the language this tick is being run in.
         * @return The committed locale.
         */
        [[nodiscard]] Locale locale() const noexcept
        {
            return active.locale();
        }

        /**
         * @brief Get the language the next tick will be run in.
         * @return The staged locale, equal to locale() unless something
         * has asked for a change during this tick.
         */
        [[nodiscard]] Locale next() const noexcept
        {
            return staged;
        }

        /**
         * @brief Ask to be in another language from the next tick on.
         * @param locale The language to change to.
         */
        void request(Locale locale) noexcept
        {
            staged = locale;
        }

        /**
         * @brief Apply a staged change at the tick boundary.
         * @param event engine.tick commits whatever was staged, and a
         * game.set_locale stages what it names; anything else is
         * ignored.
         * @throws OptionsFormatError If a game.set_locale carries a
         * payload that is not a tag this build has a catalogue for.
         */
        void handle(const TickEvent &event) override;

    private:
        Translator active;
        LanguageTranslator languageNames;
        Locale staged;
    };

} // namespace antwika::game
