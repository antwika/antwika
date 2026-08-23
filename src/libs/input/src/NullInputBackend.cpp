#include "antwika/input/NullInputBackend.hpp"

#include <optional>
#include <string_view>

#include <antwika/log/Level.hpp>

namespace antwika::input
{

    using antwika::log::Level;

    NullInputBackend::NullInputBackend(ILogger &logger)
    {
        logger.log(Level::Debug, "input.null: reporting no input");
    }

    std::string_view NullInputBackend::getName() const
    {
        return "null";
    }

    InputCapabilities NullInputBackend::getCapabilities() const
    {
        return InputCapabilities{.keyboard = true, .pointer = true};
    }

    std::optional<InputEvent> NullInputBackend::pollEvent()
    {
        return std::nullopt;
    }

}
