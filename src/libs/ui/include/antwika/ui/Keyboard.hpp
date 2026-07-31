#pragma once

#include <cstdint>
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
         */
        Activate,
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
         * @brief Compare two frames' keyboards.
         * @param other The keyboard to compare against.
         * @return True when the same keys arrived in the same order.
         */
        [[nodiscard]] bool operator==(const Keyboard &other) const =
            default;
    };

} // namespace antwika::ui
