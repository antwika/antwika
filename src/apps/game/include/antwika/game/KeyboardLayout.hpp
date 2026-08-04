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
     * @brief Which physical board the typed characters are read off.
     *
     * antwika::input reports where a key *is*, never what it types --
     * see input::Key -- so somebody has to say that the key at the
     * American slash types a hyphen on a Swedish board.
     * This is that somebody's vocabulary: the layout is simulation
     * state on the bindings' exact terms, folded in the tick path,
     * announced onto the wire at the start of a live run, and never
     * read off the window system -- see KeyboardSource.
     *
     * Deliberately separate from i18n::Locale: what the captions say
     * and what the keys type are different facts about a player, and
     * an English reader on a Swedish board is exactly who this
     * project's own author is.
     */
    enum class KeyboardLayout : std::uint8_t
    {
        /**
         * @brief The American QWERTY the input::Key names describe.
         */
        English = 0,

        /**
         * @brief The Swedish QWERTY board.
         */
        Swedish,
    };

    /**
     * @brief Every layout, in declaration order.
     */
    inline constexpr std::array<KeyboardLayout, 2> kKeyboardLayouts{
        KeyboardLayout::English, KeyboardLayout::Swedish};

    /**
     * @brief How many layouts there are.
     */
    inline constexpr std::size_t kKeyboardLayoutCount =
        kKeyboardLayouts.size();

    /**
     * @brief The layout every run begins at, on every machine.
     *
     * Swedish, because that is the board this project is written on;
     * an English machine announces its own -- see KeyboardSource.
     */
    inline constexpr KeyboardLayout kDefaultKeyboardLayout =
        KeyboardLayout::Swedish;

    /**
     * @brief Get a layout's index, for addressing a per-layout array.
     * @param layout The layout to index.
     * @return The index, always below kKeyboardLayoutCount.
     */
    [[nodiscard]] constexpr std::size_t keyboardLayoutIndex(
        KeyboardLayout layout) noexcept
    {
        return static_cast<std::size_t>(layout);
    }

    /**
     * @brief Get a layout's stable, persisted name.
     *
     * What an options file and a game.set_keyboard payload hold, so
     * these are part of both formats and may not be changed once
     * written -- the same rule actionName() is held to.
     *
     * @param layout The layout to name.
     * @return Its name, e.g. "swedish".
     */
    [[nodiscard]] std::string_view keyboardLayoutName(
        KeyboardLayout layout) noexcept;

    /**
     * @brief Get the layout a persisted name refers to.
     * @param name The name to look up, as keyboardLayoutName() made it.
     * @return The layout, or nothing for a name no layout goes by.
     */
    [[nodiscard]] std::optional<KeyboardLayout> keyboardLayoutFromName(
        std::string_view name) noexcept;

    /**
     * @brief Get what a layout is called on screen.
     * @param layout The layout to word.
     * @return The id of the caption, for a Translator to word.
     */
    [[nodiscard]] MessageId keyboardLayoutLabel(
        KeyboardLayout layout) noexcept;

} // namespace antwika::game
