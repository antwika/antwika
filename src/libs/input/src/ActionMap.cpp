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
            KeyModifiers required, KeyModifiers held) noexcept
        {
            return (!required.shift || held.shift)
                   && (!required.control || held.control)
                   && (!required.alt || held.alt)
                   && (!required.super || held.super);
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
        std::string action, Binding binding, KeyModifiers required)
    {
        if (action.empty())
        {
            throw InputError("input: an action needs a name");
        }

        bindings[std::move(action)].push_back(
            BoundInput{.binding = binding, .required = required});
    }

    bool ActionMap::isActive(
        std::string_view action, const InputState &state) const
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
                           input.required, state.keyboard().modifiers());
            });
    }

    bool ActionMap::wasTriggered(
        std::string_view action, const InputState &state) const
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
                           input.required,
                           pressModifiers(input.binding, state));
            });
    }

    const std::vector<ActionMap::BoundInput> *ActionMap::bindingsFor(
        std::string_view action) const
    {
        const auto found = bindings.find(action);
        if (found == bindings.end())
        {
            return nullptr;
        }

        return &found->second;
    }

}
