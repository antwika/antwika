#pragma once

#include <string>
#include <string_view>

#include "antwika/input/IClipboard.hpp"

namespace antwika::input
{

    /**
     * @brief A clipboard that is a string in this process.
     *
     * What the null backend answers makeSelectedClipboard() with, and
     * what a test injects: no window system, nothing shared with any
     * other program, and a paste is exactly what was last set.
     *
     * In the library rather than under backends/, for the same reason
     * NullInputBackend is: it belongs to the coverage gate.
     */
    class MemoryClipboard final : public IClipboard
    {
    public:
        /**
         * @brief Get what the clipboard holds.
         * @return The characters, empty when it holds none.
         */
        [[nodiscard]] std::string text() const override;

        /**
         * @brief Replace what the clipboard holds.
         * @param text The characters to hold.
         */
        void setText(std::string_view text) override;

    private:
        std::string held;
    };

} // namespace antwika::input
