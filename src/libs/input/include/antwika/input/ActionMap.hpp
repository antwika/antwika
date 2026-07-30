#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "antwika/input/Binding.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/KeyModifiers.hpp"

namespace antwika::input
{

    /**
     * @brief Inputs bound to the names an application knows them by.
     *
     * What makes controls rebindable, and what keeps a key name out of the
     * code that reacts to it: a system asks whether "move_left" is active,
     * not whether A is down.
     *
     * One action holds as many bindings as it likes, so WASD and the arrow
     * keys can drive the same movement. Bindings live in a std::map keyed
     * by action name, so iteration order comes from the names rather than
     * from a hash seed -- nothing here depends on that order today, and
     * nothing later should have to check whether it does.
     */
    class ActionMap final
    {
    public:
        /**
         * @brief Bind an input to an action name.
         *
         * Binding the same input to the same action twice leaves two
         * bindings, and both answer the same way, so it changes nothing
         * but the work done to ask.
         *
         * @param action The application's name for the action.
         * @param binding The key or button that triggers it.
         * @param required Modifiers that must also be held. Only the
         * modifiers set here are checked, so a binding requiring none
         * answers the same whether or not shift happens to be held --
         * which is what lets a plain "S" binding survive somebody holding
         * control for an unrelated reason.
         * @throws InputError If action is empty.
         */
        void bind(
            std::string action,
            Binding binding,
            KeyModifiers required = {});

        /**
         * @brief Check whether an action's input is currently held.
         * @param action The action to ask about.
         * @param state The folded state to read.
         * @return True when any of the action's bindings is down with its
         * required modifiers held. False for an action nothing was bound
         * to, since an unbound action is not an error to ask about.
         */
        [[nodiscard]] bool isActive(
            std::string_view action, const InputState &state) const;

        /**
         * @brief Check whether an action's input went down this tick.
         * @param action The action to ask about.
         * @param state The folded state to read.
         * @return True when any of the action's bindings was pressed this
         * tick with its required modifiers held.
         */
        [[nodiscard]] bool wasTriggered(
            std::string_view action, const InputState &state) const;

    private:
        struct BoundInput
        {
            Binding binding;
            KeyModifiers required;
        };

        [[nodiscard]] const std::vector<BoundInput> *bindingsFor(
            std::string_view action) const;

        std::map<std::string, std::vector<BoundInput>, std::less<>> bindings;
    };

} // namespace antwika::input
