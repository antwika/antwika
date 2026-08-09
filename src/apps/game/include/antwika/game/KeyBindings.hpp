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

    inline constexpr Key kQuitKey = Key::Escape;

    inline constexpr Key kFullscreenKey = Key::F10;

    inline constexpr std::array<Key, 2> kReservedKeys{
        kQuitKey, kFullscreenKey};

    [[nodiscard]] constexpr bool isReservedKey(Key key) noexcept
    {
        return key == kQuitKey || key == kFullscreenKey;
    }

    enum class BindOutcome : std::uint8_t
    {
        Bound = 0,

        Unchanged,

        Reserved,

        Taken,
    };

    class KeyBindings final
    {
    public:
        constexpr KeyBindings() noexcept = default;

        [[nodiscard]] constexpr Key keyFor(Action action) const noexcept
        {
            return keys[actionIndex(action)];
        }

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

        constexpr BindOutcome bind(Action action, Key key) noexcept
        {
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

        [[nodiscard]] bool operator==(
            const KeyBindings &other) const = default;

    private:
        std::array<Key, kActionCount> keys{
            Key::Space,
            Key::Equal,
            Key::Minus,
            Key::Home,
            Key::Grave,
            Key::Enter};
    };

    inline constexpr KeyBindings kDefaultBindings{};

    struct KeyBinding final
    {
        Action action{};

        Key key{};

        [[nodiscard]] bool operator==(
            const KeyBinding &other) const = default;
    };

}
