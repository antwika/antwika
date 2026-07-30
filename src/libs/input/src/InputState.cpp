#include "antwika/input/InputState.hpp"

#include <variant>

namespace antwika::input
{

    namespace
    {
        // One overload per alternative, not a chain of type tests.
        // A new InputEvent alternative then fails to compile here.
        // It would otherwise fold silently into nothing.
        // Dispatch also costs no branch for a test to have to reach.
        class Fold final
        {
        public:
            Fold(Keyboard &keys, Mouse &pointer) noexcept
                : keys(keys), pointer(pointer)
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
                pointer.apply(event);
            }

            void operator()(const PointerButtonPressed &event) const noexcept
            {
                keys.applyModifiers(event.modifiers);
                pointer.apply(event);
            }

            void operator()(const PointerButtonReleased &event) const noexcept
            {
                keys.applyModifiers(event.modifiers);
                pointer.apply(event);
            }

            void operator()(const PointerScrolled &event) const noexcept
            {
                pointer.apply(event);
            }

        private:
            Keyboard &keys;
            Mouse &pointer;
        };
    } // namespace

    void InputState::beginTick() noexcept
    {
        keys.beginTick();
        pointer.beginTick();
    }

    void InputState::apply(const InputEvent &event) noexcept
    {
        std::visit(Fold(keys, pointer), event);
    }

    const Keyboard &InputState::keyboard() const noexcept
    {
        return keys;
    }

    const Mouse &InputState::mouse() const noexcept
    {
        return pointer;
    }

} // namespace antwika::input
