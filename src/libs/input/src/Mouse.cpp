#include "antwika/input/Mouse.hpp"

#include <cstddef>

namespace antwika::input
{

    namespace
    {
        [[nodiscard]] bool named(std::size_t index) noexcept
        {
            return index < kMouseButtonCount;
        }
    } // namespace

    void Mouse::beginTick() noexcept
    {
        pressed.reset();
        released.reset();
        pressedWith.fill(KeyModifiers{});
        moved = Offset{};
        scrolled = Offset{};
    }

    void Mouse::apply(const PointerMoved &event) noexcept
    {
        moveTo(event.position);
    }

    void Mouse::apply(const PointerButtonPressed &event) noexcept
    {
        // A press reports where it happened, and the pointer is there.
        // The shared path keeps position() and delta() agreeing.
        moveTo(event.position);

        const auto index = mouseButtonIndex(event.button);
        if (!named(index))
        {
            return;
        }

        down.set(index);
        pressed.set(index);
        pressedWith[index] = event.modifiers;
    }

    void Mouse::apply(const PointerButtonReleased &event) noexcept
    {
        moveTo(event.position);

        const auto index = mouseButtonIndex(event.button);
        if (!named(index))
        {
            return;
        }

        down.reset(index);
        released.set(index);
    }

    void Mouse::apply(const PointerScrolled &event) noexcept
    {
        scrolled.x += event.horizontal;
        scrolled.y += event.vertical;
    }

    Position Mouse::position() const noexcept
    {
        return at;
    }

    Offset Mouse::delta() const noexcept
    {
        return moved;
    }

    Offset Mouse::scroll() const noexcept
    {
        return scrolled;
    }

    bool Mouse::isDown(MouseButton button) const noexcept
    {
        const auto index = mouseButtonIndex(button);
        return named(index) && down.test(index);
    }

    bool Mouse::anyDown() const noexcept
    {
        return down.any();
    }

    bool Mouse::wasPressed(MouseButton button) const noexcept
    {
        const auto index = mouseButtonIndex(button);
        return named(index) && pressed.test(index);
    }

    bool Mouse::wasReleased(MouseButton button) const noexcept
    {
        const auto index = mouseButtonIndex(button);
        return named(index) && released.test(index);
    }

    KeyModifiers Mouse::pressModifiers(MouseButton button) const noexcept
    {
        const auto index = mouseButtonIndex(button);
        return named(index) ? pressedWith[index] : KeyModifiers{};
    }

    void Mouse::moveTo(Position position) noexcept
    {
        if (located)
        {
            moved.x += position.x - at.x;
            moved.y += position.y - at.y;
        }

        at = position;
        located = true;
    }

} // namespace antwika::input
