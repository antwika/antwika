#pragma once

#include <cstdint>

#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Locale.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/game/Action.hpp"
#include "antwika/game/LocaleState.hpp"
#include "antwika/game/Messages.hpp"
#include "antwika/game/OptionsState.hpp"

namespace antwika::game
{

    using antwika::gfx::Size;
    using antwika::ui::Frame;
    using antwika::ui::Pointer;
    using antwika::ui::WidgetId;

    /**
     * @brief What the options screen's items are called.
     *
     * Its own namespace rather than the main menu's, for the reason
     * menuWidgets has one of its own: the two are resolved against
     * different frames and never share one, so a number colliding across
     * them would be a merge hazard and never a bug the app could see.
     *
     * None of these ever reaches a replay: what is recorded is the click
     * and the key press, and which row they landed on is worked out
     * again from them -- see MainMenuSink.
     */
    namespace optionsWidgets
    {
        /**
         * @brief Leave the options screen for the main menu.
         */
        inline constexpr WidgetId kBack{201};

        /**
         * @brief The first action's row, one per Action.
         *
         * The actions run from here in their declaration order, so an
         * action and its row cannot drift apart -- actionWidget() is the
         * one place that mapping is written.
         */
        inline constexpr WidgetId kFirstAction{210};

        /**
         * @brief Get which row asks for an action's key.
         * @param action The action to ask about.
         * @return That action's row.
         */
        [[nodiscard]] constexpr WidgetId actionWidget(
            Action action) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstAction)
                + actionIndex(action));
        }

        /**
         * @brief The first language's button, one per i18n::Locale.
         *
         * Far enough above kFirstAction that the two ranges cannot meet
         * whichever grows, which is the only thing keeping a click on a
         * language off a key binding.
         */
        inline constexpr WidgetId kFirstLanguage{260};

        /**
         * @brief Get which button picks a language.
         * @param locale The language to ask about.
         * @return That language's button.
         */
        [[nodiscard]] constexpr WidgetId languageWidget(
            antwika::i18n::Locale locale) noexcept
        {
            return static_cast<WidgetId>(
                static_cast<std::uint64_t>(kFirstLanguage)
                + static_cast<std::uint64_t>(locale));
        }
    } // namespace optionsWidgets

    /**
     * @brief The options screen: which key asks for what, and nothing
     * else.
     *
     * Drawn in place of the main menu rather than as a mode of its own,
     * because it is the same screen a player is already on with
     * something else on it -- MainMenuScene::draw() paints both, off the
     * one overlay, and the flag saying which is up is simulation state
     * in OptionsState. So there is deliberately no draw() here: a
     * second one would be a second backdrop to keep in step with the
     * first, for a screen that is the first one's other face.
     *
     * Stateless and deterministic, like MainMenuScene: the same canvas,
     * pointer and state always produce the same picture and the same
     * answer about what was pressed, which is what lets a whole screen
     * be asserted with EXPECT_EQ and no mock.
     *
     * The canvas it is laid out against must be the size the window was
     * *asked* for rather than the size one reports, for the reason
     * Toolbar gives: a hit-test is a function of the layout, and the
     * layout is a function of the canvas.
     */
    class OptionsScene final
    {
    public:
        /**
         * @brief Construct the screen over the language it words itself
         * in.
         * @param translator Words every caption; must outlive this
         * scene.
         */
        /**
         * @brief Construct the scene over what words it.
         * @param translator The captions; must outlive this object.
         * @param languages The language names, at the same locale; must
         * outlive this object. A second translator rather than more ids
         * because `Locale` is i18n's enum and its names are i18n's --
         * see LocaleState.hpp.
         */
        OptionsScene(
            const Translator &translator,
            const LanguageTranslator &languages);

        /**
         * @brief Describe the screen for one tick.
         * @param canvas The area the screen is laid out into.
         * @param pointer Where the pointer is and what it is doing.
         * @param state The bindings, the waiting action and whatever the
         * last attempt had to say.
         * @param active Which language is on, so its button can be shown
         * as the one already chosen. Passed rather than read off the
         * translator so this stays a pure function of its arguments.
         * @return The drawing commands and what the pointer did.
         */
        [[nodiscard]] Frame describe(
            Size canvas,
            Pointer pointer,
            const OptionsState &state,
            antwika::i18n::Locale active) const;

    private:
        const Translator &translator;
        const LanguageTranslator &languages;
    };

} // namespace antwika::game
