#pragma once

/**
 * @file
 * @brief Names of the events this library encodes input as.
 *
 * These travel through the same event mechanism every other event uses,
 * which is what lets a recorded interactive session be replayed: a
 * `--record` run persists these, and nothing else about the input.
 *
 * They are part of the replay format, so a name here may not be changed
 * once a replay has been written with it.
 */
namespace antwika::input::events
{

    /**
     * @brief A key went down.
     *
     * The payload is a JSON object with a symbolic "key" name (as
     * produced by toString(Key)), the four modifier flags "shift",
     * "control", "alt" and "super", and a "repeat" flag saying whether
     * the window system generated this from a held key.
     *
     * Symbolic rather than a platform scancode, deliberately: a raw
     * keycode would make a replay specific to the backend that recorded
     * it.
     */
    inline constexpr const char *kKeyDown = "input.key_down";

    /**
     * @brief A key came back up.
     *
     * The same payload as kKeyDown without the "repeat" flag, which a
     * release does not have.
     */
    inline constexpr const char *kKeyUp = "input.key_up";

    /**
     * @brief The pointer moved.
     *
     * The payload is a JSON object with signed integer "x" and "y"
     * fields, in the backend's own surface coordinates -- see
     * antwika::input::Position for what that does and does not promise.
     */
    inline constexpr const char *kPointerMove = "input.pointer_move";

    /**
     * @brief A pointer button went down.
     *
     * The payload is a JSON object with a symbolic "button" name (as
     * produced by toString(MouseButton)), the "x" and "y" of
     * kPointerMove, and the four modifier flags of kKeyDown.
     */
    inline constexpr const char *kPointerDown = "input.pointer_down";

    /**
     * @brief A pointer button came back up.
     *
     * The same payload as kPointerDown.
     */
    inline constexpr const char *kPointerUp = "input.pointer_up";

    /**
     * @brief A scroll wheel turned.
     *
     * The payload is a JSON object with signed integer "horizontal" and
     * "vertical" notch counts.
     */
    inline constexpr const char *kPointerScroll = "input.pointer_scroll";

} // namespace antwika::input::events
