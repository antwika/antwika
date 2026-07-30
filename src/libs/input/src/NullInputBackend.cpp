#include "antwika/input/NullInputBackend.hpp"

#include <optional>
#include <string_view>

#include <antwika/log/Level.hpp>

namespace antwika::input
{

    using antwika::log::Level;

    NullInputBackend::NullInputBackend(ILogger &logger)
    {
        // Logged here rather than kept.
        // Nothing after construction has anything to say.
        // A stored logger nothing reads is a field the compiler flags.
        logger.log(Level::Debug, "input.null: reporting no input");
    }

    std::string_view NullInputBackend::name() const
    {
        return "null";
    }

    InputCapabilities NullInputBackend::capabilities() const
    {
        return InputCapabilities{.keyboard = true, .pointer = true};
    }

    std::optional<InputEvent> NullInputBackend::pollEvent()
    {
        return std::nullopt;
    }

} // namespace antwika::input
