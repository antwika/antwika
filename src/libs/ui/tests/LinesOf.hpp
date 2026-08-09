#pragma once

#include <cstddef>
#include <string>

namespace antwika::ui::tests
{

    [[nodiscard]] inline std::string linesOf(const std::size_t lines)
    {
        std::string text;

        for (std::size_t at = 0; at < lines; ++at)
        {
            text += static_cast<char>('a' + (at % 26));

            if (at + 1 < lines)
            {
                text += '\n';
            }
        }

        return text;
    }

}
