#include "antwika/input/ActionMap.hpp"

#include <algorithm>
#include <utility>
#include <variant>

#include "antwika/input/InputError.hpp"

namespace antwika::input
{

    namespace
    {
        [[nodiscard]] bool satisfies(
            KeyModifiers requiredModifiers,
            KeyModifiers heldModifiers,
            ExtraModifiers extras) noexcept
        {
            if (extras == ExtraModifiers::Refused)
            {
                return requiredModifiers == heldModifiers;
            }

            return (!requiredModifiers.shift || heldModifiers.shift)
                   && (!requiredModifiers.control || heldModifiers.control)
                   && (!requiredModifiers.alt || heldModifiers.alt)
                   && (!requiredModifiers.super || heldModifiers.super);
        }

        [[nodiscard]] bool isDown(
            Binding binding, const InputState &state) noexcept
        {
            if (const auto *key = std::get_if<Key>(&binding))
            {
                return state.keyboard().isDown(*key);
            }

            return state.mouse().isDown(std::get<MouseButton>(binding));
        }

        [[nodiscard]] bool wasPressed(
            Binding binding, const InputState &state) noexcept
        {
            if (const auto *key = std::get_if<Key>(&binding))
            {
                return state.keyboard().wasPressed(*key);
            }

            return state.mouse().wasPressed(std::get<MouseButton>(binding));
        }

        [[nodiscard]] KeyModifiers pressModifiers(
            Binding binding, const InputState &state) noexcept
        {
            if (const auto *key = std::get_if<Key>(&binding))
            {
                return state.keyboard().pressModifiers(*key);
            }

            return state.mouse().pressModifiers(
                std::get<MouseButton>(binding));
        }
    }

    void ActionMap::bind(
        std::string action,
        Binding binding,
        KeyModifiers requiredModifiers,
        ExtraModifiers extras)
    {
        if (action.empty())
        {
            throw InputError("input: an action needs a name");
        }

        boundInputs[std::move(action)].push_back(
            BoundInput{.binding = binding,
                .requiredModifiers = requiredModifiers,
                .extras = extras});
    }

    void ActionMap::rebind(
        std::string action,
        Binding binding,
        KeyModifiers requiredModifiers,
        ExtraModifiers extras)
    {
        unbind(action);
        bind(std::move(action), binding, requiredModifiers, extras);
    }

    void ActionMap::unbind(const std::string_view action)
    {
        const auto foundInput = boundInputs.find(action);
        if (foundInput != boundInputs.end())
        {
            boundInputs.erase(foundInput);
        }
    }

    bool ActionMap::isBound(const std::string_view action) const
    {
        const auto *bound = bindingsFor(action);

        return bound != nullptr && !bound->empty();
    }

    bool ActionMap::matches(
        const std::string_view action,
        const Binding binding,
        const KeyModifiers heldModifiers) const
    {
        const auto *bound = bindingsFor(action);
        if (bound == nullptr)
        {
            return false;
        }

        return std::ranges::any_of(
            *bound,
            [binding, heldModifiers](const BoundInput &input)
            {
                return input.binding == binding
                       && satisfies(
                           input.requiredModifiers,
                           heldModifiers,
                           input.extras);
            });
    }

    bool ActionMap::isActive(
        const std::string_view action, const InputState &state) const
    {
        const auto *bound = bindingsFor(action);
        if (bound == nullptr)
        {
            return false;
        }

        return std::ranges::any_of(
            *bound,
            [&state](const BoundInput &input)
            {
                return isDown(input.binding, state)
                       && satisfies(
                           input.requiredModifiers,
                           state.keyboard().modifiers(),
                           input.extras);
            });
    }

    bool ActionMap::wasTriggered(
        const std::string_view action, const InputState &state) const
    {
        const auto *bound = bindingsFor(action);
        if (bound == nullptr)
        {
            return false;
        }

        return std::ranges::any_of(
            *bound,
            [&state](const BoundInput &input)
            {
                return wasPressed(input.binding, state)
                       && satisfies(
                           input.requiredModifiers,
                           pressModifiers(input.binding, state),
                           input.extras);
            });
    }

    const std::vector<ActionMap::BoundInput> *ActionMap::bindingsFor(
        const std::string_view action) const
    {
        const auto foundInput = boundInputs.find(action);
        if (foundInput == boundInputs.end())
        {
            return nullptr;
        }

        return &foundInput->second;
    }

}
