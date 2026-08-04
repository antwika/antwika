#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include <antwika/event/ITickEventSource.hpp>
#include <antwika/input/Key.hpp>

#include "antwika/game/Action.hpp"

namespace antwika::game
{

    using antwika::event::ITickEventSource;
    using antwika::input::Key;

    /**
     * @brief The key that ends a live run.
     *
     * Acted on by input::InputPipeline, upstream of every sink, so
     * nothing in the tick path can stop it meaning that -- which is
     * exactly why it may not be bound to anything else.
     */
    inline constexpr Key kQuitKey = Key::Escape;

    /**
     * @brief The key that fills the screen with the window.
     *
     * Acted on by app::FullscreenToggleSource, above the loop, for the
     * reason wiki/apps/game.md gives; a binding on it would fire and
     * fill the screen as well, which is nobody's idea of a key binding.
     */
    inline constexpr Key kFullscreenKey = Key::F10;

    /**
     * @brief The keys this application acts on outside the tick path.
     *
     * Named here rather than only in main() so that the options screen
     * can refuse them: a binding this list holds would be one that
     * always did two things, and one of them would not be the one the
     * player asked for.
     */
    inline constexpr std::array<Key, 2> kReservedKeys{
        kQuitKey, kFullscreenKey};

    /**
     * @brief Check whether a key is one this application already spends
     * outside the tick path.
     * @param key The key to ask about.
     * @return True when nothing may be bound to it.
     */
    [[nodiscard]] constexpr bool isReservedKey(Key key) noexcept
    {
        return key == kQuitKey || key == kFullscreenKey;
    }

    /**
     * @brief What became of an attempt to bind a key.
     *
     * A total answer rather than a throw, because binding happens while
     * a frame is being described: a screen has to be able to say what
     * happened and carry on drawing.
     */
    enum class BindOutcome : std::uint8_t
    {
        /**
         * @brief The action now answers to that key.
         */
        Bound = 0,

        /**
         * @brief It already did, so nothing changed.
         */
        Unchanged,

        /**
         * @brief The key is one this application spends above the tick
         * path -- see kReservedKeys.
         */
        Reserved,

        /**
         * @brief Another action holds that key.
         *
         * Refused rather than stolen. Stealing would leave the other
         * action with no key at all, which would make an action's key a
         * std::optional everywhere for the sake of one gesture, and a
         * player who has just taken a key off something would have to be
         * told which. Two rebindings say the same thing and say it in
         * the order the player chose.
         */
        Taken,
    };

    /**
     * @brief Which key asks for which action, for one run.
     *
     * **A plain value, and that is the whole design.** It is copied into
     * a run, folded by the tick path and compared in GameSummary, so a
     * live run and its replay disagreeing about a binding is a
     * divergence the replay comparison catches like any other.
     *
     * A run always begins at kDefaultBindings, a constant fixed in
     * source, exactly as the locale, the canvas and the world seed are
     * -- and for the same reason each of those was made a constant
     * rather than a flag. Nothing about the machine reaches this
     * directly: what the player's own options file holds enters a run as
     * game.bind_key events through an ITickEventSource, so a recording
     * carries it and a replay reproduces it -- see BindingSource.hpp.
     *
     * That is what answers the objection sudoku::digitFor states: which
     * key does what is still a layout written down rather than asked of
     * a window system, and what a recording holds is still the symbolic
     * key. The layout is simply the session's rather than the build's,
     * and the session carries it.
     *
     * Every action always has a key, and no two actions share one, which
     * bind() is what maintains.
     *
     * **Not input::ActionMap**, which is the library's own rebindable
     * map and answers a different question. That one holds as many
     * bindings per action as a caller likes and only ever gains them:
     * there is no unbind, so an action cannot be *re*bound, which is the
     * whole of what an options screen does. It is also keyed by a
     * std::string name and is not comparable, so it could be neither a
     * constant fixed in source nor a member of GameSummary. What is
     * wanted here is a small total value with exactly one key per
     * action, and this is that.
     */
    class KeyBindings final
    {
    public:
        /**
         * @brief Construct the layout this application ships with.
         */
        constexpr KeyBindings() noexcept = default;

        /**
         * @brief Get which key asks for an action.
         * @param action The action to ask about.
         * @return Its key, which every action always has.
         */
        [[nodiscard]] constexpr Key keyFor(Action action) const noexcept
        {
            return keys[actionIndex(action)];
        }

        /**
         * @brief Get which action a key asks for.
         * @param key The key that went down.
         * @return The action, or nothing for a key nothing is bound to.
         */
        [[nodiscard]] constexpr std::optional<Action> actionFor(
            Key key) const noexcept
        {
            for (const auto action : kActions)
            {
                if (keyFor(action) == key)
                {
                    return action;
                }
            }

            return std::nullopt;
        }

        /**
         * @brief Make a key ask for an action.
         *
         * Total: every pair of arguments has an answer and none throws.
         *
         * @param action The action to rebind.
         * @param key The key to bind it to.
         * @return What became of it; the bindings are unchanged for
         * anything but BindOutcome::Bound.
         */
        constexpr BindOutcome bind(Action action, Key key) noexcept
        {
            // Asked first.
            // So rebinding an action to the key it already holds.
            // Is not reported as somebody else holding it.
            if (keyFor(action) == key)
            {
                return BindOutcome::Unchanged;
            }

            if (isReservedKey(key))
            {
                return BindOutcome::Reserved;
            }

            if (actionFor(key).has_value())
            {
                return BindOutcome::Taken;
            }

            keys[actionIndex(action)] = key;
            return BindOutcome::Bound;
        }

        /**
         * @brief Compare two layouts.
         * @param other The layout to compare against.
         * @return True when every action answers to the same key.
         */
        [[nodiscard]] bool operator==(
            const KeyBindings &other) const = default;

    private:
        // Space pauses because it is the biggest key on the board.
        // And the run is what a player holds and lets go of most.
        // The zoom pair is the one every map in the world uses.
        // None of the four is reserved, and no two are the same.
        // KeyBindingsTest asserts that rather than a comment.
        std::array<Key, kActionCount> keys{
            Key::Space, Key::Equal, Key::Minus, Key::Home};
    };

    /**
     * @brief The layout every run begins at, on every machine.
     *
     * The one thing that makes a recording mean the same everywhere: a
     * run that was never told otherwise is playing this, so a recording
     * made on a machine that had never opened the options screen holds
     * no binding at all and replays under any machine's.
     */
    inline constexpr KeyBindings kDefaultBindings{};

    /**
     * @brief One action and the key that asks for it.
     *
     * What a game.bind_key payload and one line of an options file each
     * carry; a whole KeyBindings is a list of these.
     */
    struct KeyBinding
    {
        /** @brief The action being bound. */
        Action action{};

        /** @brief The key it answers to. */
        Key key{};

        /**
         * @brief Compare two bindings.
         * @param other The binding to compare against.
         * @return True when both members match.
         */
        [[nodiscard]] bool operator==(
            const KeyBinding &other) const = default;
    };

} // namespace antwika::game
