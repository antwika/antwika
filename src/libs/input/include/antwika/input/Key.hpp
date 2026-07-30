#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace antwika::input
{

    /**
     * @brief A key on a keyboard, identified by its role.
     *
     * Named after what the key is rather than the character it produces.
     * The same key yields a different character under a different layout,
     * and an application binding "move forward" wants the key next to S
     * whatever that key happens to type.
     *
     * Values are contiguous from zero, so a set of held keys can be a
     * bitset indexed by keyIndex(). That indexing is in-memory only --
     * replays persist the names from toString(), not the numbers -- so
     * this list may be reordered freely, as long as the last enumerator
     * stays last, since kKeyCount is derived from it.
     */
    enum class Key : std::uint8_t
    {
        A = 0,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,

        Digit0,
        Digit1,
        Digit2,
        Digit3,
        Digit4,
        Digit5,
        Digit6,
        Digit7,
        Digit8,
        Digit9,

        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,

        ArrowLeft,
        ArrowRight,
        ArrowUp,
        ArrowDown,

        Escape,
        Enter,
        Space,
        Tab,
        Backspace,
        Delete,
        Insert,
        Home,
        End,
        PageUp,
        PageDown,

        Minus,
        Equal,
        LeftBracket,
        RightBracket,
        Backslash,
        Semicolon,
        Apostrophe,
        Grave,
        Comma,
        Period,
        Slash,

        CapsLock,
        LeftShift,
        RightShift,
        LeftControl,
        RightControl,
        LeftAlt,
        RightAlt,
        LeftSuper,
        RightSuper,
    };

    /**
     * @brief How many keys this vocabulary holds.
     *
     * Derived from the last enumerator rather than written out, so it
     * cannot drift from the enumeration it counts.
     */
    inline constexpr std::size_t kKeyCount =
        static_cast<std::size_t>(Key::RightSuper) + 1;

    /**
     * @brief Get a key's index, for addressing a per-key array.
     * @param key The key to index.
     * @return The index, always below kKeyCount for a named key.
     */
    [[nodiscard]] constexpr std::size_t keyIndex(Key key) noexcept
    {
        return static_cast<std::size_t>(key);
    }

    /**
     * @brief Get a key's stable, persisted name.
     *
     * This is what a replay stores, so these names are part of the replay
     * format and may not be changed once written.
     *
     * A value with no name is refused rather than named "Unknown".
     * keyFromString rejects a name no key goes by, so the lenient answer
     * would write a recording that encodes cleanly and then fails to
     * replay -- in another process, long after the cause was visible.
     *
     * @param key The key to name.
     * @return The key's name, e.g. "Escape".
     * @throws InputError If the value is outside the enumeration, which
     * only a cast can produce.
     */
    [[nodiscard]] std::string_view toString(Key key);

    /**
     * @brief Get the key a persisted name refers to.
     * @param name The name to look up, as produced by toString().
     * @return The key that name refers to.
     * @throws InputError If no key goes by that name.
     */
    [[nodiscard]] Key keyFromString(std::string_view name);

} // namespace antwika::input
