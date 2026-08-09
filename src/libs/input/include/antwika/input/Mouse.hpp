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

    class Mouse final
    {
    public:
        void beginTick() noexcept;

        void apply(const PointerMoved &event) noexcept;

        void apply(const PointerButtonPressed &event) noexcept;

        void apply(const PointerButtonReleased &event) noexcept;

        void apply(const PointerScrolled &event) noexcept;

        [[nodiscard]] Position position() const noexcept;

        [[nodiscard]] Offset delta() const noexcept;

        [[nodiscard]] Offset scroll() const noexcept;

        [[nodiscard]] bool isDown(MouseButton button) const noexcept;

        [[nodiscard]] bool anyDown() const noexcept;

        [[nodiscard]] bool wasPressed(MouseButton button) const noexcept;

        [[nodiscard]] bool wasReleased(MouseButton button) const noexcept;

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

        bool located = false;
    };

}
