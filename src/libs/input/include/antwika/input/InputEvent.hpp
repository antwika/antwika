#pragma once

#include <cstdint>
#include <variant>

#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    /**
     * @brief A key went down.
     */
    struct KeyPressed
    {
        Key key = Key::A;
        KeyModifiers modifiers{};

        /**
         * @brief Whether the window system generated this from a key being
         * held rather than from a fresh press.
         *
         * A repeat is what makes a held key type a run of characters. Held
         * state is already available from the folded device state, so
         * anything reacting to a press rather than to typing should ignore
         * a repeat.
         */
        bool repeat = false;

        /**
         * @brief Compare two presses.
         * @param other The press to compare against.
         * @return True when the key, modifiers and repeat flag all match.
         */
        [[nodiscard]] bool operator==(const KeyPressed &other) const = default;
    };

    /**
     * @brief A key came back up.
     */
    struct KeyReleased
    {
        Key key = Key::A;
        KeyModifiers modifiers{};

        /**
         * @brief Compare two releases.
         * @param other The release to compare against.
         * @return True when both the key and the modifiers match.
         */
        [[nodiscard]] bool operator==(const KeyReleased &other) const = default;
    };

    /**
     * @brief The pointer moved to a new position.
     */
    struct PointerMoved
    {
        Position position{};

        /**
         * @brief Compare two movements.
         * @param other The movement to compare against.
         * @return True when both report the same position.
         */
        [[nodiscard]] bool operator==(
            const PointerMoved &other) const = default;
    };

    /**
     * @brief A pointer button went down.
     */
    struct PointerButtonPressed
    {
        MouseButton button = MouseButton::Left;
        Position position{};
        KeyModifiers modifiers{};

        /**
         * @brief Compare two presses.
         * @param other The press to compare against.
         * @return True when the button, position and modifiers all match.
         */
        [[nodiscard]] bool operator==(
            const PointerButtonPressed &other) const = default;
    };

    /**
     * @brief A pointer button came back up.
     */
    struct PointerButtonReleased
    {
        MouseButton button = MouseButton::Left;
        Position position{};
        KeyModifiers modifiers{};

        /**
         * @brief Compare two releases.
         * @param other The release to compare against.
         * @return True when the button, position and modifiers all match.
         */
        [[nodiscard]] bool operator==(
            const PointerButtonReleased &other) const = default;
    };

    /**
     * @brief A scroll wheel turned.
     *
     * Whole notches, not fractions: a trackpad reporting a continuous
     * gesture is the backend's problem to accumulate into notches, so that
     * what a replay stores is an integer count rather than a float whose
     * rounding could differ between a live run and its replay.
     *
     * Positive vertical is away from the user; positive horizontal is to
     * the right.
     */
    struct PointerScrolled
    {
        std::int32_t horizontal = 0;
        std::int32_t vertical = 0;

        /**
         * @brief Compare two scrolls.
         * @param other The scroll to compare against.
         * @return True when both axes match.
         */
        [[nodiscard]] bool operator==(
            const PointerScrolled &other) const = default;
    };

    /**
     * @brief Something a keyboard or a pointer did.
     *
     * Every alternative is an edge -- a press, a release, a move, a notch
     * -- and never a statement of what is currently held. Held state is
     * derived above this seam by folding these events, which is what lets
     * a framework with an event queue and a framework with only pollable
     * state both report through the same type, and what keeps a replay to
     * the external input it must persist rather than the state it can
     * regenerate.
     */
    using InputEvent = std::variant<
        KeyPressed,
        KeyReleased,
        PointerMoved,
        PointerButtonPressed,
        PointerButtonReleased,
        PointerScrolled>;

} // namespace antwika::input
