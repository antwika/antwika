#pragma once

#include <optional>

#include <antwika/i18n/Locale.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/KeyBindings.hpp"
#include "antwika/game/KeyboardLayout.hpp"

namespace antwika::game
{

    /**
     * @brief Everything the options screen remembers, and the run's key
     * bindings with it.
     *
     * **All of it is simulation state**, in exactly the sense the camera
     * and the app's mode are (see Camera.hpp and blog/013): whether the
     * screen is up decides what a click at a pixel means, which action
     * is waiting decides what a key press means, and the bindings decide
     * what every later key press means. A renderer that owned any of the
     * three would leave a replay resolving recorded input against a
     * different screen.
     *
     * It is therefore folded by the tick path and never persisted as
     * itself: MainMenuSink resolves the clicks and the key presses
     * against the screen's layout, downstream of the recorder, and a
     * replay works the same answers out again. No `ui.*` event exists
     * for any of it, and none may.
     *
     * The one thing that does arrive as an event is the layout the
     * *machine* was carrying when the run started, because that is the
     * one part of this nothing in the run can work out again -- see
     * BindingSource.hpp.
     */
    class OptionsState final
    {
    public:
        /**
         * @brief Construct the state a run opens with: the screen down,
         * nothing waiting, and the bindings this application ships.
         */
        OptionsState() noexcept = default;

        OptionsState(const OptionsState &) = delete;
        OptionsState(OptionsState &&) = delete;

        OptionsState &operator=(const OptionsState &) = delete;
        OptionsState &operator=(OptionsState &&) = delete;

        /**
         * @brief Check whether the options screen is showing.
         * @return True while it is up, in place of the main menu.
         */
        [[nodiscard]] bool open() const noexcept;

        /**
         * @brief Show or put away the options screen.
         *
         * Putting it away forgets whichever action was waiting for a
         * key, so a screen opened again is not still half-way through a
         * gesture nobody finished.
         *
         * @param showing Whether the screen is up.
         */
        void setOpen(bool showing) noexcept;

        /**
         * @brief Get the run's key bindings.
         * @return The layout every bound key is read through.
         */
        [[nodiscard]] const KeyBindings &bindings() const noexcept;

        /**
         * @brief Get which action is waiting to be told a key.
         * @return The action, or nothing when none is.
         */
        [[nodiscard]] std::optional<Action> awaiting() const noexcept;

        /**
         * @brief Wait for a key on behalf of an action.
         *
         * Clears whatever the last attempt had to say, so a stale
         * refusal is not still on screen under a fresh question.
         *
         * @param action The action the next key press will be bound to.
         */
        void await(Action action) noexcept;

        /**
         * @brief Get what the last attempt to bind a key came to.
         * @return The outcome, or nothing when nothing has been tried
         * since the screen last asked a question.
         */
        [[nodiscard]] std::optional<BindOutcome> notice() const noexcept;

        /**
         * @brief Offer a key to whichever action is waiting for one.
         *
         * A refusal leaves the action waiting, so a player who reached
         * for a key that was taken can simply reach for another; only an
         * accepted key ends the gesture.
         *
         * @param key The key that went down.
         * @return What became of it, or nothing when no action was
         * waiting -- which is an ordinary state rather than a mistake,
         * since every key press on the screen arrives here.
         */
        std::optional<BindOutcome> press(Key key) noexcept;

        /**
         * @brief Bind a key without anything having asked for one.
         *
         * What the machine's own layout arrives through, applied one
         * binding at a time as BindingSink folds each game.bind_key.
         * It touches neither the waiting action nor the notice, since
         * neither is a thing the player did.
         *
         * @param action The action to bind.
         * @param key The key to bind it to.
         * @return What became of it.
         */
        BindOutcome apply(Action action, Key key) noexcept;

        /**
         * @brief Get the language the player has picked.
         *
         * Held here so it can be written to the options file beside the
         * bindings, and read back to announce on the next run. What the
         * run is *actually* being played in is LocaleState's, which is a
         * sink in the tick path -- this is the preference, that is the
         * simulation state, and MainMenuSink sets both from one press.
         *
         * @return The picked language.
         */
        [[nodiscard]] antwika::i18n::Locale locale() const noexcept;

        /**
         * @brief Remember the language the player has picked.
         * @param locale The language to write down.
         */
        void setLocale(antwika::i18n::Locale locale) noexcept;

        /**
         * @brief Get the board the run's typing is read off.
         *
         * Simulation state exactly as the bindings are: what a key
         * press *types* depends on it, so it is folded in the tick
         * path, announced at a live run's start by KeyboardSource,
         * and changed by a recorded click on the options screen.
         *
         * @return The layout every typed character goes through.
         */
        [[nodiscard]] KeyboardLayout keyboard() const noexcept;

        /**
         * @brief Take the board the player, or an announcement, named.
         * @param layout The layout typing now goes through.
         */
        void setKeyboard(KeyboardLayout layout) noexcept;

    private:
        KeyBindings current{};
        std::optional<Action> pending{};
        std::optional<BindOutcome> last{};
        bool showing = false;
        antwika::i18n::Locale language{antwika::i18n::kDefaultLocale};
        KeyboardLayout typing{kDefaultKeyboardLayout};
    };

} // namespace antwika::game
