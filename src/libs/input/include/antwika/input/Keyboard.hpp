#pragma once

#include <array>
#include <bitset>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"

namespace antwika::input
{

    /**
     * @brief What the keyboard is doing, folded from the edges it
     * reported.
     *
     * IInputBackend reports only edges -- a press, a release -- because
     * that is what both an event-queue framework and a state-polling one
     * can honestly produce, and because only edges are external input a
     * replay has to persist. This is where those edges become the
     * questions application code actually asks.
     *
     * "This tick" means since the last beginTick(). Clearing the edges is
     * the caller's job rather than a side effect of reading them: an edge
     * that cleared itself on the first read would give a different answer
     * to the second reader, and two systems asking about the same key
     * would then depend on which ran first.
     *
     * A Key outside the enumeration is ignored rather than trusted. It can
     * only arrive from a cast, but a bitset written past its end is memory
     * corruption, which is too high a price for trusting a caller.
     */
    class Keyboard final
    {
    public:
        /**
         * @brief Forget this tick's edges, keeping what is still held.
         */
        void beginTick() noexcept;

        /**
         * @brief Fold a key going down into the state.
         *
         * A repeat marks the key held and modifiers current, but is not a
         * press: the window system generated it from a key nobody let go
         * of, so anything reacting to a press rather than to typing would
         * fire over and over on one keystroke.
         *
         * @param event The press to apply.
         */
        void apply(const KeyPressed &event) noexcept;

        /**
         * @brief Fold a key coming back up into the state.
         * @param event The release to apply.
         */
        void apply(const KeyReleased &event) noexcept;

        /**
         * @brief Fold the modifiers an event was carrying.
         *
         * Public because a pointer event carries them too, and a held
         * modifier belongs to the keyboard whichever event mentioned it --
         * see InputState, which is what routes them here.
         *
         * @param held The modifiers that were held.
         */
        void applyModifiers(KeyModifiers held) noexcept;

        /**
         * @brief Check whether a key is currently held.
         * @param key The key to ask about.
         * @return True while it is down.
         */
        [[nodiscard]] bool isDown(Key key) const noexcept;

        /**
         * @brief Check whether a key went down this tick.
         * @param key The key to ask about.
         * @return True if it was pressed since the last beginTick(),
         * ignoring repeats.
         */
        [[nodiscard]] bool wasPressed(Key key) const noexcept;

        /**
         * @brief Check whether a key came up this tick.
         * @param key The key to ask about.
         * @return True if it was released since the last beginTick().
         */
        [[nodiscard]] bool wasReleased(Key key) const noexcept;

        /**
         * @brief Get the modifiers held as of the last event to say.
         * @return The modifiers.
         */
        [[nodiscard]] KeyModifiers modifiers() const noexcept;

        /**
         * @brief Get the modifiers a key's own press edge carried.
         *
         * modifiers() is the tick's last word on what was held, which is
         * the wrong question to ask about a chord: a tick is a window
         * several edges arrive in, so Ctrl+S with Ctrl let go afterwards
         * and S pressed before Ctrl went down both fold to the same
         * end-of-tick state and mean opposite things.
         *
         * Every edge carries its own modifiers, so this keeps the press
         * edge's and hands them back beside wasPressed().
         *
         * @param key The key to ask about.
         * @return What was held when it went down this tick, or nothing
         * held when it did not go down this tick at all.
         */
        [[nodiscard]] KeyModifiers pressModifiers(Key key) const noexcept;

    private:
        std::bitset<kKeyCount> down;
        std::bitset<kKeyCount> pressed;
        std::bitset<kKeyCount> released;
        std::array<KeyModifiers, kKeyCount> pressedWith{};
        KeyModifiers held;
    };

} // namespace antwika::input
