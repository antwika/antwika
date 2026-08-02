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
    } // namespace

    void Keyboard::beginTick() noexcept
    {
        pressed.reset();
        released.reset();
        pressedWith.fill(KeyModifiers{});
    }

    void Keyboard::apply(const KeyPressed &event) noexcept
    {
        applyModifiers(event.modifiers);

        const auto index = keyIndex(event.key);
        if (!named(index))
        {
            return;
        }

        down.set(index);

        if (!event.repeat)
        {
            pressed.set(index);
            pressedWith[index] = event.modifiers;
        }
    }

    void Keyboard::apply(const KeyReleased &event) noexcept
    {
        applyModifiers(event.modifiers);

        const auto index = keyIndex(event.key);
        if (!named(index))
        {
            return;
        }

        down.reset(index);
        released.set(index);
    }

    void Keyboard::applyModifiers(KeyModifiers modifiers) noexcept
    {
        held = modifiers;
    }

    bool Keyboard::isDown(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) && down.test(index);
    }

    bool Keyboard::wasPressed(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) && pressed.test(index);
    }

    bool Keyboard::wasReleased(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) && released.test(index);
    }

    KeyModifiers Keyboard::modifiers() const noexcept
    {
        return held;
    }

    KeyModifiers Keyboard::pressModifiers(Key key) const noexcept
    {
        const auto index = keyIndex(key);
        return named(index) ? pressedWith[index] : KeyModifiers{};
    }

} // namespace antwika::input
