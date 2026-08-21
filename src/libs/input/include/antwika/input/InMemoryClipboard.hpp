#pragma once

#include <string>
#include <string_view>

#include "antwika/input/IClipboard.hpp"

namespace antwika::input
{

    class InMemoryClipboard final : public IClipboard
    {
    public:
        [[nodiscard]] std::string text() const override;

        void setText(std::string_view text) override;

    private:
        std::string heldText;
    };

}
