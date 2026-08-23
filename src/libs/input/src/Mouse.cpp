#include "antwika/input/Mouse.hpp"

#include <cstddef>

namespace antwika::input
{

    namespace
    {
        [[nodiscard]] bool isNamed(std::size_t index) noexcept
        {
            return index < kMouseButtonCount;
        }
    }

    void Mouse::beginTick() noexcept
    {
        pressedButtons.reset();
        releasedButtons.reset();
        pressedWithModifiers.fill(KeyModifiers{});
        movedOffset = Offset{};
        scrolledOffset = Offset{};
    }

    void Mouse::apply(const PointerMoved &eventMoved) noexcept
    {
        moveTo(eventMoved.position);
    }

    void Mouse::apply(const PointerButtonPressed &eventPressed) noexcept
    {
        moveTo(eventPressed.position);

        const auto index = getMouseButtonIndex(eventPressed.button);
        if (!isNamed(index))
        {
            return;
        }

        downButtons.set(index);
        pressedButtons.set(index);
        pressedWithModifiers[index] = eventPressed.modifiers;
    }

    void Mouse::apply(const PointerButtonReleased &eventReleased) noexcept
    {
        moveTo(eventReleased.position);

        const auto index = getMouseButtonIndex(eventReleased.button);
        if (!isNamed(index))
        {
            return;
        }

        downButtons.reset(index);
        releasedButtons.set(index);
    }

    void Mouse::apply(const PointerScrolled &eventScrolled) noexcept
    {
        scrolledOffset.x += eventScrolled.horizontal;
        scrolledOffset.y += eventScrolled.vertical;
    }

    Position Mouse::getPosition() const noexcept
    {
        return pointerPosition;
    }

    Offset Mouse::getDelta() const noexcept
    {
        return movedOffset;
    }

    Offset Mouse::getScroll() const noexcept
    {
        return scrolledOffset;
    }

    bool Mouse::isDown(MouseButton button) const noexcept
    {
        const auto index = getMouseButtonIndex(button);
        return isNamed(index) && downButtons.test(index);
    }

    bool Mouse::isAnyDown() const noexcept
    {
        return downButtons.any();
    }

    bool Mouse::wasPressed(MouseButton button) const noexcept
    {
        const auto index = getMouseButtonIndex(button);
        return isNamed(index) && pressedButtons.test(index);
    }

    bool Mouse::wasReleased(MouseButton button) const noexcept
    {
        const auto index = getMouseButtonIndex(button);
        return isNamed(index) && releasedButtons.test(index);
    }

    KeyModifiers Mouse::getPressModifiers(MouseButton button) const noexcept
    {
        const auto index = getMouseButtonIndex(button);
        return isNamed(index) ? pressedWithModifiers[index] : KeyModifiers{};
    }

    void Mouse::moveTo(Position position) noexcept
    {
        if (located)
        {
            movedOffset.x += position.x - pointerPosition.x;
            movedOffset.y += position.y - pointerPosition.y;
        }

        pointerPosition = position;
        located = true;
    }

}
