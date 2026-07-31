#pragma once

#include <string_view>

namespace antwika::ui
{

    /**
     * @brief What the caller reports about typing, for one frame.
     *
     * Pointer.hpp's counterpart, and for exactly the same reason: this
     * library reads no device, so an application folds whatever its
     * input backend reported into a value and hands it across.
     *
     * Every field is an edge that arrived during this frame, never a
     * statement of what is currently held, which is what antwika::input
     * already promises about an InputEvent.
     *
     * A default-constructed TextInput reports that nothing was typed, so
     * a UI built without one behaves exactly as it did before there was
     * one.
     *
     * Deliberately the smallest set a text field needs.
     * Anything wider -- a general key value, focus traversal, a modifier
     * -- belongs in the keyboard input value this is expected to be
     * folded into later, not here.
     */
    struct TextInput
    {
        /**
         * @brief The characters that arrived this frame, in order.
         *
         * A view rather than a string: the caller owns the buffer and
         * must keep it alive for as long as the Context is.
         */
        std::string_view typed{};

        /**
         * @brief Whether Backspace arrived this frame.
         */
        bool backspace = false;

        /**
         * @brief Whether Enter arrived this frame.
         */
        bool submit = false;

        /**
         * @brief Whether Escape arrived this frame.
         */
        bool cancel = false;

        /**
         * @brief Whether the left arrow arrived this frame.
         */
        bool left = false;

        /**
         * @brief Whether the right arrow arrived this frame.
         */
        bool right = false;
    };

} // namespace antwika::ui
