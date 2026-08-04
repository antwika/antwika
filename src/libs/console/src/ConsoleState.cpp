#include "antwika/console/ConsoleState.hpp"

#include <utility>

#include <antwika/animation/Progress.hpp>
#include <antwika/tween/Ease.hpp>
#include <antwika/tween/Easing.hpp>

namespace antwika::console
{

    namespace
    {
        // Fast at first and settling at the end, both directions.
        // Cubic rather than sine, since a curve here has to be exact.
        constexpr antwika::tween::Easing kConsoleEasing =
            antwika::tween::Easing::CubicOut;
    } // namespace

    std::uint32_t consoleHeightAt(std::uint32_t steps, Size canvas)
    {
        // Cubing the fraction cubes the denominator.
        // Eight cubed is 512, which interpolate() takes in stride.
        // So this call site needs no guard against tween::TweenError.
        const auto eased = antwika::tween::ease(
            kConsoleEasing,
            antwika::animation::Progress(steps, kConsoleAnimTicks));

        return static_cast<std::uint32_t>(antwika::animation::interpolate(
            0, static_cast<std::int64_t>(canvas.height / 2), eased));
    }

    void ConsoleState::toggle() noexcept
    {
        wanted = !wanted;
    }

    void ConsoleState::advance() noexcept
    {
        if (wanted && along < kConsoleAnimTicks)
        {
            ++along;
        }
        else if (!wanted && along > 0)
        {
            --along;
        }
    }

    bool ConsoleState::visible() const noexcept
    {
        return along > 0;
    }

    bool ConsoleState::acceptsText() const noexcept
    {
        return wanted && along == kConsoleAnimTicks;
    }

    std::uint32_t ConsoleState::steps() const noexcept
    {
        return along;
    }

    void ConsoleState::setHeight(std::uint32_t pixels) noexcept
    {
        tall = pixels;
    }

    std::uint32_t ConsoleState::height() const noexcept
    {
        return tall;
    }

    bool ConsoleState::covers(Point at) const noexcept
    {
        return visible() && at.y < static_cast<std::int32_t>(tall);
    }

    const std::string &ConsoleState::line() const noexcept
    {
        return typed;
    }

    std::size_t ConsoleState::caret() const noexcept
    {
        return cursor;
    }

    void ConsoleState::setLine(std::string text, std::size_t cursor)
    {
        typed = std::move(text);
        this->cursor = cursor;
    }

    std::string ConsoleState::takeLine()
    {
        auto taken = std::move(typed);
        typed.clear();
        cursor = antwika::ui::kCaretAtEnd;

        return taken;
    }

    const std::vector<std::string> &ConsoleState::history() const noexcept
    {
        return lines;
    }

    void ConsoleState::pushHistory(std::string entry)
    {
        lines.push_back(std::move(entry));
    }

    void ConsoleState::replaceHistory(std::vector<std::string> loaded)
    {
        lines = std::move(loaded);
    }

} // namespace antwika::console
