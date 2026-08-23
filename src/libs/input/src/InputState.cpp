#include "antwika/input/InputState.hpp"

#include <variant>

namespace antwika::input
{

    namespace
    {
        class Fold final
        {
        public:
            Fold(Keyboard &keyboard, Mouse &pointerMouse) noexcept
                : keys(keyboard), pointerMouse(pointerMouse)
            {
            }

            Fold(const Fold &) = delete;
            Fold(Fold &&) = delete;

            Fold &operator=(const Fold &) = delete;
            Fold &operator=(Fold &&) = delete;

            void operator()(const KeyPressed &event) const noexcept
            {
                keys.apply(event);
            }

            void operator()(const KeyReleased &event) const noexcept
            {
                keys.apply(event);
            }

            void operator()(const PointerMoved &event) const noexcept
            {
                pointerMouse.apply(event);
            }

            void operator()(const PointerButtonPressed &event) const noexcept
            {
                keys.applyModifiers(event.modifiers);
                pointerMouse.apply(event);
            }

            void operator()(const PointerButtonReleased &event) const noexcept
            {
                keys.applyModifiers(event.modifiers);
                pointerMouse.apply(event);
            }

            void operator()(const PointerScrolled &event) const noexcept
            {
                pointerMouse.apply(event);
            }

        private:
            Keyboard &keys;
            Mouse &pointerMouse;
        };
    }

    void InputState::beginTick() noexcept
    {
        keys.beginTick();
        pointerMouse.beginTick();
    }

    void InputState::apply(const InputEvent &event) noexcept
    {
        std::visit(Fold(keys, pointerMouse), event);
    }

    const Keyboard &InputState::getKeyboard() const noexcept
    {
        return keys;
    }

    const Mouse &InputState::getMouse() const noexcept
    {
        return pointerMouse;
    }

}
