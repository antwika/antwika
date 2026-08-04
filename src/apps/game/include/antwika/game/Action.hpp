#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "antwika/game/MessageId.hpp"

namespace antwika::game
{

    /**
     * @brief Something a key can be made to do.
     *
     * The half of a binding that is *not* a key: an action is what this
     * application does, named once here, and which key asks for it is a
     * separate question KeyBindings answers.
     *
     * Every action listed here is resolved inside the tick path and
     * changes what the run computes, which is deliberate: an options
     * screen over actions no recording could contain would be safe and
     * would also not be the feature. What keeps that safe instead is
     * that a binding is regenerated from the recording rather than read
     * off the machine -- see KeyBindings.hpp.
     *
     * Values are contiguous from zero so a per-action array can be
     * indexed by actionIndex(); the names below are what a persisted
     * binding holds, so they may not be changed once written.
     */
    enum class Action : std::uint8_t
    {
        /**
         * @brief Hold the city still, or let it run again.
         */
        Pause = 0,

        /**
         * @brief Take the camera one whole tile size closer.
         */
        ZoomIn,

        /**
         * @brief Take the camera one whole tile size further away.
         */
        ZoomOut,

        /**
         * @brief Put the camera back where the run opened it.
         */
        ResetView,

        /**
         * @brief Slide the debug console in, or send it back out.
         */
        ConsoleToggle,

        /**
         * @brief Execute what the console's field holds.
         */
        ConsoleExecute,
    };

    /**
     * @brief Every action, in declaration order.
     *
     * What an options screen lists and what a file is written out of, so
     * that adding an action adds a row and a line rather than an edit in
     * each.
     */
    inline constexpr std::array<Action, 6> kActions{
        Action::Pause,
        Action::ZoomIn,
        Action::ZoomOut,
        Action::ResetView,
        Action::ConsoleToggle,
        Action::ConsoleExecute};

    /**
     * @brief How many actions there are.
     */
    inline constexpr std::size_t kActionCount = kActions.size();

    /**
     * @brief Get an action's index, for addressing a per-action array.
     * @param action The action to index.
     * @return The index, always below kActionCount.
     */
    [[nodiscard]] constexpr std::size_t actionIndex(Action action) noexcept
    {
        return static_cast<std::size_t>(action);
    }

    /**
     * @brief Get an action's stable, persisted name.
     *
     * What an options file and a game.bind_key payload hold, so these
     * are part of both formats and may not be changed once written --
     * the same rule input::toString(Key) is held to.
     *
     * Not a caption: a name a person reads goes through
     * antwika::i18n, and actionLabel() is that.
     *
     * @param action The action to name.
     * @return Its name, e.g. "zoom_in".
     */
    [[nodiscard]] std::string_view actionName(Action action) noexcept;

    /**
     * @brief Get the action a persisted name refers to.
     * @param name The name to look up, as actionName() produced it.
     * @return The action, or nothing for a name no action goes by.
     */
    [[nodiscard]] std::optional<Action> actionFromName(
        std::string_view name) noexcept;

    /**
     * @brief Get what an action is called on screen.
     * @param action The action to word.
     * @return The id of the caption, for a Translator to word.
     */
    [[nodiscard]] MessageId actionLabel(
        Action action) noexcept;

} // namespace antwika::game
