#pragma once

#include <string>
#include <string_view>

namespace antwika::input
{

    /**
     * @brief The window system's clipboard, behind the backend seam.
     *
     * Two calls, because a clipboard is two flows and each has one
     * lawful place in the stack.  **Reading it is input**: what it
     * holds came from outside the run, cannot be worked out again, and
     * so may only enter a simulation upstream of the recorder, as an
     * event the recording carries -- exactly as a key press does.
     * **Writing it is a projection**: an outward copy of state the run
     * already owns, read back by nothing, on the same terms a frame is
     * drawn.
     *
     * A backend with no window system behind it answers with
     * MemoryClipboard, a string in this process -- so a headless run
     * still pastes what it copied, and a test needs no display.
     */
    class IClipboard
    {
    public:
        virtual ~IClipboard() = default;

        /**
         * @brief Get what the clipboard holds.
         * @return The characters, empty when it holds none.
         */
        [[nodiscard]] virtual std::string text() const = 0;

        /**
         * @brief Replace what the clipboard holds.
         * @param text The characters to hold.
         */
        virtual void setText(std::string_view text) = 0;
    };

} // namespace antwika::input
