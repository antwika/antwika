#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/ui/TextFieldSpec.hpp>

namespace antwika::console
{

    using antwika::gfx::Point;
    using antwika::gfx::Size;

    inline constexpr std::uint32_t kConsoleAnimTicks = 8;

    [[nodiscard]] std::uint32_t consoleHeightAt(
        std::uint32_t steps, Size canvas);

    class ConsoleState final
    {
    public:
        ConsoleState() noexcept = default;

        ConsoleState(const ConsoleState &) = delete;
        ConsoleState(ConsoleState &&) = delete;

        ConsoleState &operator=(const ConsoleState &) = delete;
        ConsoleState &operator=(ConsoleState &&) = delete;

        void toggle() noexcept;

        void advance() noexcept;

        [[nodiscard]] bool visible() const noexcept;

        [[nodiscard]] bool acceptsText() const noexcept;

        [[nodiscard]] std::uint32_t steps() const noexcept;

        void setHeight(std::uint32_t pixels) noexcept;

        [[nodiscard]] std::uint32_t height() const noexcept;

        [[nodiscard]] bool covers(Point at) const noexcept;

        [[nodiscard]] const std::string &line() const noexcept;

        [[nodiscard]] std::size_t caret() const noexcept;

        void setLine(std::string text, std::size_t cursor);

        [[nodiscard]] std::string takeLine();

        [[nodiscard]] const std::vector<std::string> &
        history() const noexcept;

        void pushHistory(std::string entry);

        void replaceHistory(std::vector<std::string> lines);

        void rememberCommand(std::string command);

        [[nodiscard]] const std::vector<std::string> &
        commands() const noexcept;

        /**
         * @brief Walks the remembered commands into the input field.
         *
         * @param back True to step towards older commands, false
         *             towards newer ones.
         *
         * Ensures: stepping past the newest command empties the field.
         */
        void recall(bool back);

    private:
        bool wanted = false;
        std::uint32_t along = 0;
        std::uint32_t tall = 0;
        std::string typed;
        std::size_t cursor = antwika::ui::kCaretAtEnd;
        std::vector<std::string> lines;
        std::vector<std::string> entered;
        std::size_t recalled = 0;
    };

}
