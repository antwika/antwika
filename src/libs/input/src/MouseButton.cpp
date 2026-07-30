#include "antwika/input/MouseButton.hpp"

#include <array>
#include <string>
#include <string_view>

#include "antwika/input/InputError.hpp"

namespace antwika::input
{

    namespace
    {
        struct MouseButtonName
        {
            MouseButton button;
            std::string_view name;
        };

        // One table drives both directions.
        // So a name can never map to a button other than the one that made it.
        constexpr std::array<MouseButtonName, kMouseButtonCount>
            kMouseButtonNames{{
                {MouseButton::Left, "Left"},
                {MouseButton::Middle, "Middle"},
                {MouseButton::Right, "Right"},
                {MouseButton::X1, "X1"},
                {MouseButton::X2, "X2"},
            }};
    } // namespace

    std::string_view toString(MouseButton button) noexcept
    {
        for (const auto &entry : kMouseButtonNames)
        {
            if (entry.button == button)
            {
                return entry.name;
            }
        }

        return "Unknown";
    }

    MouseButton mouseButtonFromString(std::string_view name)
    {
        for (const auto &entry : kMouseButtonNames)
        {
            if (entry.name == name)
            {
                return entry.button;
            }
        }

        throw InputError("input: unknown mouse button name '"
                         + std::string(name) + "'");
    }

} // namespace antwika::input
