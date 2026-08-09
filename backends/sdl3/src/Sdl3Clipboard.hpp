#pragma once

#include <string>
#include <string_view>

#include <antwika/input/IClipboard.hpp>
#include <antwika/log/ILogger.hpp>

#include "Sdl3Runtime.hpp"

namespace antwika::input::sdl3
{

    using antwika::log::ILogger;
    using antwika::sdl3::Sdl3Subsystem;

    class Sdl3Clipboard final : public IClipboard
    {
    public:
        explicit Sdl3Clipboard(ILogger &logger);

        [[nodiscard]] std::string text() const override;

        void setText(std::string_view text) override;

    private:
        ILogger &logger;
        Sdl3Subsystem video;
    };

}
