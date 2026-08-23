#include "antwika/input/Keyboard.hpp"

#include <cstddef>

namespace antwika::input
{

    namespace
    {
        [[nodiscard]] bool isNamed(std::size_t index) noexcept
        {
            return index < kKeyCount;
        }
    }

    void Keyboard::beginTick() noexcept
    {
        pressedKeys.reset();
        releasedKeys.reset();
        pressedWithModifiers.fill(KeyModifiers{});
    }

    void Keyboard::apply(const KeyPressed &eventPressed) noexcept
    {
        applyModifiers(eventPressed.modifiers);

        const auto index = getKeyIndex(eventPressed.key);
        if (!isNamed(index))
        {
            return;
        }

        downKeys.set(index);

        if (!eventPressed.repeat)
        {
            pressedKeys.set(index);
            pressedWithModifiers[index] = eventPressed.modifiers;
        }
    }

    void Keyboard::apply(const KeyReleased &eventReleased) noexcept
    {
        applyModifiers(eventReleased.modifiers);

        const auto index = getKeyIndex(eventReleased.key);
        if (!isNamed(index))
        {
            return;
        }

        downKeys.reset(index);
        releasedKeys.set(index);
    }

    void Keyboard::applyModifiers(KeyModifiers modifiers) noexcept
    {
        heldModifiers = modifiers;
    }

    bool Keyboard::isDown(Key key) const noexcept
    {
        const auto index = getKeyIndex(key);
        return isNamed(index) && downKeys.test(index);
    }

    bool Keyboard::wasPressed(Key key) const noexcept
    {
        const auto index = getKeyIndex(key);
        return isNamed(index) && pressedKeys.test(index);
    }

    bool Keyboard::wasReleased(Key key) const noexcept
    {
        const auto index = getKeyIndex(key);
        return isNamed(index) && releasedKeys.test(index);
    }

    KeyModifiers Keyboard::getModifiers() const noexcept
    {
        return heldModifiers;
    }

    KeyModifiers Keyboard::getPressModifiers(Key key) const noexcept
    {
        const auto index = getKeyIndex(key);
        return isNamed(index) ? pressedWithModifiers[index] : KeyModifiers{};
    }

}
