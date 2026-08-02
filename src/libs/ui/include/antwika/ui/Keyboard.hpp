#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace antwika::ui
{

    /**
     * @brief One key edge this library has a meaning for.
     *
     * Symbolic, and defined here rather than taken from a framework or
     * from antwika::input, for the same reason ui::Pointer is: this
     * library reads no device and depends on antwika::gfx alone.
     * An application translates whatever its backend reports into these,
     * exactly as it folds pointer edges into a ui::Pointer.
     *
     * Only the keys the UI itself acts on are named.
     * A key an application handles on its own never needs to be here,
     * and a widget that grows a new one -- a list that walks with the
     * arrows, a field that leaves with Escape -- adds an enumerator here
     * rather than a second input channel.
     */
    enum class Key : std::uint8_t
    {
        /**
         * @brief Move focus to the next widget: Tab.
         */
        FocusNext = 0,

        /**
         * @brief Move focus to the previous widget: Shift+Tab.
         *
         * A separate key rather than a modifier flag, because a modifier
         * is a held state and everything crossing this seam is an edge.
         */
        FocusPrevious,

        /**
         * @brief Activate whatever is focused: Enter.
         *
         * Named for what it does rather than for the key it usually is,
         * since an application is free to bind Space to it as well.
         * A focused text field reads the same edge as its submit.
         */
        Activate,

        /**
         * @brief Take the character before a text field's caret.
         */
        Backspace,

        /**
         * @brief Give up on what a text field is holding: Escape.
         *
         * What giving up means is the application's to decide, since
         * only it knows what the field held before it was opened.
         */
        Cancel,

        /**
         * @brief Move a text field's caret one character back.
         */
        MoveLeft,

        /**
         * @brief Move a text field's caret one character on.
         */
        MoveRight,

        /**
         * @brief Move a text area's caret to the line above.
         *
         * To the same column, or to the end of that line when it is
         * shorter. A text field is one line, so this does nothing to
         * one.
         */
        MoveUp,

        /**
         * @brief Move a text area's caret to the line below.
         */
        MoveDown,

        /**
         * @brief Move the caret to the start of its line: Home.
         *
         * In a text field, whose one line is the whole text, that is
         * the start of the text.
         */
        MoveLineStart,

        /**
         * @brief Move the caret to the end of its line: End.
         */
        MoveLineEnd,

        /**
         * @brief Move the caret one character back, selecting as it
         * goes: Shift+Left.
         *
         * Four separate keys rather than one modifier flag beside the
         * four above, for the reason FocusPrevious is a key: a modifier
         * is held state, and everything crossing this seam is an edge.
         *
         * What each of them does to the far end of the selection is
         * nothing: that end is the caller's anchor, and only a move
         * that does *not* select brings it along. See TextEdit::anchor.
         */
        SelectLeft,

        /**
         * @brief Move the caret one character on, selecting: Shift+Right.
         */
        SelectRight,

        /**
         * @brief Move the caret to the line above, selecting: Shift+Up.
         */
        SelectUp,

        /**
         * @brief Move the caret to the line below, selecting: Shift+Down.
         */
        SelectDown,

        /**
         * @brief Move the caret to the start of its line, selecting:
         * Shift+Home.
         */
        SelectLineStart,

        /**
         * @brief Move the caret to the end of its line, selecting:
         * Shift+End.
         */
        SelectLineEnd,

        /**
         * @brief Take the character the caret sits before, or the
         * selection: Delete.
         *
         * Backspace's other half, and the one thing that makes a
         * selection worth having on its own: what Backspace and this
         * both do to a selection is take all of it.
         */
        Delete,

        /**
         * @brief Report the selected characters without changing them:
         * Ctrl+C.
         *
         * **What holds them afterwards is the caller's**, exactly as
         * the characters themselves are: a clipboard inside this
         * library would be state a replay could not regenerate. They
         * come back through TextEdit::copied, and pasting them is the
         * caller putting them in Keyboard::typed like any other
         * characters somebody produced.
         */
        Copy,

        /**
         * @brief Report the selected characters and take them out:
         * Ctrl+X.
         */
        Cut,

        /**
         * @brief Take the next character out of Keyboard::typed.
         *
         * What puts a typed character in the same ordered list as every
         * other edge, so a field replays what actually happened rather
         * than an approximation of it. See Keyboard::typed.
         */
        Character,
    };

    /**
     * @brief What the caller reports about the keyboard, for one frame.
     *
     * A list of edges rather than a set of held flags, because the order
     * two keys arrived in is the difference between tabbing away from a
     * button and then pressing Enter, and pressing Enter and then tabbing
     * away. Holding a key down is not expressible on purpose: a UI acts
     * on the press, and repeat is the application's to decide.
     *
     * A default-constructed Keyboard reports no keys at all, so a UI
     * built without one behaves exactly as it did before there was one.
     */
    struct Keyboard
    {
        /**
         * @brief The keys that went down this frame, in arrival order.
         */
        std::vector<Key> keys{};

        /**
         * @brief The characters that arrived this frame, in order.
         *
         * **Each one is taken by a Key::Character edge in the list
         * above**, which is what fixes the order the whole frame is
         * read in: a caller that folds a tick's typing into one frame
         * pushes a Character edge where each character arrived, and
         * typing `a`, Backspace, `b` comes out as `b` rather than as
         * `a`. A character with no edge to take it is not typed at all,
         * since nothing says where in the order it belongs.
         *
         * A view rather than a string: the caller owns the buffer and
         * must keep it alive for as long as the Context is.
         */
        std::string_view typed{};

        /**
         * @brief Compare two frames' keyboards.
         * @param other The keyboard to compare against.
         * @return True when the same keys arrived in the same order.
         */
        [[nodiscard]] bool operator==(const Keyboard &other) const =
            default;
    };

} // namespace antwika::ui
