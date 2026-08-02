#pragma once

#include <string>
#include <string_view>

#include <antwika/input/IClipboard.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::input::raylib
{

    using antwika::log::ILogger;

    /**
     * @brief IClipboard backed by raylib's clipboard.
     *
     * raylib keeps the clipboard on its one window, so both calls
     * answer for nothing until that window is up -- a read is empty
     * and a write is dropped, the same no-window answer the input
     * backend's polling gives.
     */
    class RaylibClipboard final : public IClipboard
    {
    public:
        /**
         * @brief Construct the clipboard.
         * @param logger Receives one line saying it is in use.
         */
        explicit RaylibClipboard(ILogger &logger);

        /**
         * @brief Get what the window system's clipboard holds.
         * @return The characters, empty when it holds none.
         */
        [[nodiscard]] std::string text() const override;

        /**
         * @brief Replace what the window system's clipboard holds.
         * @param text The characters to hold.
         */
        void setText(std::string_view text) override;
    };

} // namespace antwika::input::raylib
