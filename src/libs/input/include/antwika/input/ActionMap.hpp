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

    class ActionMap final
    {
    public:
        void bind(
            std::string action,
            Binding binding,
            KeyModifiers required = {});

        [[nodiscard]] bool isActive(
            std::string_view action, const InputState &state) const;

        [[nodiscard]] bool wasTriggered(
            std::string_view action, const InputState &state) const;

    private:
        struct BoundInput final
        {
            Binding binding;
            KeyModifiers required;
        };

        [[nodiscard]] const std::vector<BoundInput> *bindingsFor(
            std::string_view action) const;

        std::map<std::string, std::vector<BoundInput>, std::less<>> bindings;
    };

}
