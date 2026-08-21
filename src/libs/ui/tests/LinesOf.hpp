#pragma once

#include <cstddef>
#include <string>

namespace antwika::ui::tests
{

    [[nodiscard]] inline std::string linesOf(const std::size_t lines)
    {
        std::string text;

        for (std::size_t index = 0; index < lines; ++index)
        {
            text += static_cast<char>('a' + (index % 26));

            if (index + 1 < lines)
            {
                text += '\n';
            }
        }

        return text;
    }

}
