#pragma once

#include <array>
#include <bitset>

#include "antwika/input/InputEvent.hpp"
#include "antwika/input/Key.hpp"
#include "antwika/input/KeyModifiers.hpp"

namespace antwika::input
{

    class Keyboard final
    {
    public:
        void beginTick() noexcept;

        void apply(const KeyPressed &eventPressed) noexcept;

        void apply(const KeyReleased &eventReleased) noexcept;

        void applyModifiers(KeyModifiers modifiers) noexcept;

        [[nodiscard]] bool isDown(Key key) const noexcept;

        [[nodiscard]] bool wasPressed(Key key) const noexcept;

        [[nodiscard]] bool wasReleased(Key key) const noexcept;

        [[nodiscard]] KeyModifiers modifiers() const noexcept;

        [[nodiscard]] KeyModifiers pressModifiers(Key key) const noexcept;

    private:
        std::bitset<kKeyCount> downKeys;
        std::bitset<kKeyCount> pressedKeys;
        std::bitset<kKeyCount> releasedKeys;
        std::array<KeyModifiers, kKeyCount> pressedWithModifiers{};
        KeyModifiers heldModifiers;
    };

}
