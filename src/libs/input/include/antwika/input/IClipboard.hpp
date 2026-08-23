#pragma once

#include <string>
#include <string_view>

namespace antwika::input
{

    class IClipboard
    {
    public:
        virtual ~IClipboard() = default;

        [[nodiscard]] virtual std::string getText() const = 0;

        virtual void setText(std::string_view text) = 0;
    };

}
