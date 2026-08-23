#include "antwika/input/MouseButton.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "antwika/input/InputError.hpp"

namespace antwika::input
{

    namespace
    {
        struct MouseButtonName final
        {
            MouseButton button;
            std::string_view name;
        };

        constexpr std::array<MouseButtonName, kMouseButtonCount>
            kMouseButtonNames{{
                {MouseButton::Left, "Left"},
                {MouseButton::Middle, "Middle"},
                {MouseButton::Right, "Right"},
                {MouseButton::X1, "X1"},
                {MouseButton::X2, "X2"},
            }};

        [[nodiscard]] consteval bool namesEveryButtonExactlyOnce()
        {
            for (std::size_t index = 0; index < kMouseButtonCount; ++index)
            {
                std::size_t rows = 0;

                for (const auto &entry : kMouseButtonNames)
                {
                    if (getMouseButtonIndex(entry.button) == index)
                    {
                        ++rows;
                    }
                }

                if (rows != 1)
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] consteval bool namesEveryButtonDistinctly()
        {
            const auto count = kMouseButtonNames.size();

            for (std::size_t left = 0; left < count; ++left)
            {
                for (std::size_t right = left + 1; right < count; ++right)
                {
                    const auto &names = kMouseButtonNames;

                    if (names[left].name == names[right].name)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        static_assert(
            namesEveryButtonExactlyOnce(),
            "kMouseButtonNames must name every MouseButton exactly once");
        static_assert(
            namesEveryButtonDistinctly(),
            "kMouseButtonNames must not give two buttons the same name");
    }

    std::string_view toString(MouseButton button)
    {
        for (const auto &entry : kMouseButtonNames)
        {
            if (entry.button == button)
            {
                return entry.name;
            }
        }

        throw InputError(
            "input: no name for mouse button "
            + std::to_string(getMouseButtonIndex(button)));
    }

    MouseButton getMouseButtonFromString(std::string_view name)
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

}
