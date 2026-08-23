#pragma once

#include <string>
#include <string_view>

#include <antwika/input/IClipboard.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::input::raylib
{

    using antwika::log::ILogger;

    class RaylibClipboard final : public IClipboard
    {
    public:
        explicit RaylibClipboard(ILogger &logger);

        [[nodiscard]] std::string getText() const override;

        void setText(std::string_view text) override;
    };

}
