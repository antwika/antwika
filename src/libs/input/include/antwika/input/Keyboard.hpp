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

        void apply(const KeyPressed &event) noexcept;

        void apply(const KeyReleased &event) noexcept;

        void applyModifiers(KeyModifiers held) noexcept;

        [[nodiscard]] bool isDown(Key key) const noexcept;

        [[nodiscard]] bool wasPressed(Key key) const noexcept;

        [[nodiscard]] bool wasReleased(Key key) const noexcept;

        [[nodiscard]] KeyModifiers modifiers() const noexcept;

        [[nodiscard]] KeyModifiers pressModifiers(Key key) const noexcept;

    private:
        std::bitset<kKeyCount> down;
        std::bitset<kKeyCount> pressed;
        std::bitset<kKeyCount> released;
        std::array<KeyModifiers, kKeyCount> pressedWith{};
        KeyModifiers held;
    };

}
