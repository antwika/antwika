#include "antwika/input/Keyboard.hpp"

#include <cstddef>

namespace antwika::input
{

    namespace
    {
        [[nodiscard]] bool named(std::size_t index) noexcept
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

        const auto index = keyIndex(eventPressed.key);
        if (!named(index))
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

        const auto index = keyIndex(eventReleased.key);
        if (!named(index))
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
        const auto index = keyIndex(key);
        return named(index) && downKeys.test(index);
    }

    bool Keyboard::wasPressed(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) && pressedKeys.test(index);
    }

    bool Keyboard::wasReleased(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) && releasedKeys.test(index);
    }

    KeyModifiers Keyboard::modifiers() const noexcept
    {
        return heldModifiers;
    }

    KeyModifiers Keyboard::pressModifiers(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) ? pressedWithModifiers[index] : KeyModifiers{};
    }

}
