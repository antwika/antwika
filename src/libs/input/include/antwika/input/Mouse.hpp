#pragma once

#include <array>
#include <bitset>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/KeyModifiers.hpp"
#include "antwika/input/MouseButton.hpp"
#include "antwika/input/Offset.hpp"
#include "antwika/input/Position.hpp"

namespace antwika::input
{

    /**
     * @brief What the pointer is doing, folded from the edges it reported.
     *
     * The pointer's half of what Keyboard does for keys, on the same
     * terms: edges in, questions out, and "this tick" meaning since the
     * last beginTick().
     *
     * A MouseButton outside the enumeration is ignored, for the reason
     * Keyboard gives.
     */
    class Mouse final
    {
    public:
        /**
         * @brief Forget this tick's edges, movement and scrolling.
         *
         * The position survives, because where the pointer is is not an
         * edge. The delta and the scroll do not, because both are
         * per-tick sums.
         */
        void beginTick() noexcept;

        /**
         * @brief Fold a movement into the state.
         *
         * The very first position reported produces no delta. There is
         * nothing to have moved from, and treating the default-constructed
         * origin as a previous position would report a jump from the
         * window's corner to wherever the pointer actually was.
         *
         * @param event The movement to apply.
         */
        void apply(const PointerMoved &event) noexcept;

        /**
         * @brief Fold a button going down into the state.
         * @param event The press to apply.
         */
        void apply(const PointerButtonPressed &event) noexcept;

        /**
         * @brief Fold a button coming back up into the state.
         * @param event The release to apply.
         */
        void apply(const PointerButtonReleased &event) noexcept;

        /**
         * @brief Fold a scroll into this tick's running total.
         * @param event The scroll to apply.
         */
        void apply(const PointerScrolled &event) noexcept;

        /**
         * @brief Get where the pointer was last reported to be.
         * @return The position, or a default-constructed one if nothing
         * has reported one yet.
         */
        [[nodiscard]] Position position() const noexcept;

        /**
         * @brief Get how far the pointer moved this tick.
         * @return The sum of this tick's movements.
         */
        [[nodiscard]] Offset delta() const noexcept;

        /**
         * @brief Get how far the wheel turned this tick.
         * @return The sum of this tick's notches, x horizontal, y
         * vertical.
         */
        [[nodiscard]] Offset scroll() const noexcept;

        /**
         * @brief Check whether a button is currently held.
         * @param button The button to ask about.
         * @return True while it is down.
         */
        [[nodiscard]] bool isDown(MouseButton button) const noexcept;

        /**
         * @brief Check whether any button at all is currently held.
         *
         * The question a gesture asks before it asks which button: a
         * movement means something while anything is down, and means
         * nothing while nothing is.
         *
         * @return True while at least one button is down.
         */
        [[nodiscard]] bool anyDown() const noexcept;

        /**
         * @brief Check whether a button went down this tick.
         * @param button The button to ask about.
         * @return True if it was pressed since the last beginTick().
         */
        [[nodiscard]] bool wasPressed(MouseButton button) const noexcept;

        /**
         * @brief Check whether a button came up this tick.
         * @param button The button to ask about.
         * @return True if it was released since the last beginTick().
         */
        [[nodiscard]] bool wasReleased(MouseButton button) const noexcept;

        /**
         * @brief Get the modifiers a button's own press edge carried.
         *
         * The pointer's half of Keyboard::pressModifiers(), for the
         * reason given there: a chord means what was held at the press,
         * not what was left held when the tick ended.
         *
         * @param button The button to ask about.
         * @return What was held when it went down this tick, or nothing
         * held when it did not go down this tick at all.
         */
        [[nodiscard]] KeyModifiers pressModifiers(
            MouseButton button) const noexcept;

    private:
        void moveTo(Position position) noexcept;

        std::bitset<kMouseButtonCount> down;
        std::bitset<kMouseButtonCount> pressed;
        std::bitset<kMouseButtonCount> released;
        std::array<KeyModifiers, kMouseButtonCount> pressedWith{};

        Position at;
        Offset moved;
        Offset scrolled;

        // Whether anything has reported a position yet.
        // Without this the first movement reads as a delta from (0, 0).
        bool located = false;
    };

} // namespace antwika::input
