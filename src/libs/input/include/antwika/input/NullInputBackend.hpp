#pragma once

#include <optional>
#include <string_view>

#include <antwika/log/ILogger.hpp>

#include "antwika/input/IInputBackend.hpp"
#include "antwika/input/InputCapabilities.hpp"
#include "antwika/input/InputEvent.hpp"

namespace antwika::input
{

    using antwika::log::ILogger;

    class NullInputBackend final : public IInputBackend
    {
    public:
        explicit NullInputBackend(ILogger &logger);

        NullInputBackend(const NullInputBackend &) = delete;
        NullInputBackend(NullInputBackend &&) = delete;

        NullInputBackend &operator=(const NullInputBackend &) = delete;
        NullInputBackend &operator=(NullInputBackend &&) = delete;

        [[nodiscard]] std::string_view name() const override;

        [[nodiscard]] InputCapabilities capabilities() const override;

        [[nodiscard]] std::optional<InputEvent> pollEvent() override;
    };

}
