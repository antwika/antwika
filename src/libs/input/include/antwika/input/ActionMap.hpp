#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "antwika/input/Binding.hpp"
#include "antwika/input/ExtraModifiers.hpp"
#include "antwika/input/InputState.hpp"
#include "antwika/input/KeyModifiers.hpp"

namespace antwika::input
{

    class ActionMap final
    {
    public:
        void bind(
            std::string action,
            Binding binding,
            KeyModifiers requiredModifiers = {},
            ExtraModifiers extras = ExtraModifiers::Allowed);

        void rebind(
            std::string action,
            Binding binding,
            KeyModifiers requiredModifiers = {},
            ExtraModifiers extras = ExtraModifiers::Allowed);

        void unbind(std::string_view action);

        [[nodiscard]] bool isBound(std::string_view action) const;

        [[nodiscard]] bool matches(
            std::string_view action,
            Binding binding,
            KeyModifiers heldModifiers) const;

        [[nodiscard]] bool isActive(
            std::string_view action, const InputState &state) const;

        [[nodiscard]] bool wasTriggered(
            std::string_view action, const InputState &state) const;

    private:
        struct BoundInput final
        {
            Binding binding;
            KeyModifiers requiredModifiers;
            ExtraModifiers extras = ExtraModifiers::Allowed;
        };

        [[nodiscard]] const std::vector<BoundInput> *bindingsFor(
            std::string_view action) const;

        std::map<std::string, std::vector<BoundInput>, std::less<>> boundInputs;
    };

}
