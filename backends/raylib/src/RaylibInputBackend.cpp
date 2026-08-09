#include "RaylibInputBackend.hpp"

#include <raylib.h>

#include <utility>

#include <antwika/log/Level.hpp>

#include "RaylibFrame.hpp"

namespace antwika::input::raylib
{

    using antwika::log::Level;

    namespace
    {
        constexpr std::array<std::pair<MouseButton, int>, kMouseButtonCount>
            kButtons{{
                {MouseButton::Left, MOUSE_BUTTON_LEFT},
                {MouseButton::Middle, MOUSE_BUTTON_MIDDLE},
                {MouseButton::Right, MOUSE_BUTTON_RIGHT},
                {MouseButton::X1, MOUSE_BUTTON_SIDE},
                {MouseButton::X2, MOUSE_BUTTON_EXTRA},
            }};

        [[nodiscard]] Position positionNow()
        {
            const Vector2 position = GetMousePosition();

            return Position{
                .x = static_cast<std::int32_t>(position.x),
                .y = static_cast<std::int32_t>(position.y)};
        }
    }

    RaylibInputBackend::RaylibInputBackend(ILogger &logger)
    {
        logger.log(Level::Debug, "input.raylib: reading the mouse");
    }

    std::string_view RaylibInputBackend::name() const
    {
        return "raylib";
    }

    InputCapabilities RaylibInputBackend::capabilities() const
    {
        return InputCapabilities{.keyboard = false, .pointer = true};
    }

    std::optional<InputEvent> RaylibInputBackend::pollEvent()
    {
        if (pending.empty())
        {
            sample();
        }

        if (pending.empty())
        {
            return std::nullopt;
        }

        auto event = pending.front();
        pending.pop_front();

        return event;
    }

    void RaylibInputBackend::sample()
    {
        if (!IsWindowReady())
        {
            return;
        }

        const auto position = positionNow();

        if (!lastPosition)
        {
            lastPosition = position;
        }
        else if (*lastPosition != position)
        {
            lastPosition = position;
            pending.push_back(PointerMoved{.position = position});
        }

        for (const auto &[button, raylibButton] : kButtons)
        {
            const bool down = IsMouseButtonDown(raylibButton);
            auto &wasDown = held[mouseButtonIndex(button)];

            if (down == wasDown)
            {
                continue;
            }

            wasDown = down;

            if (down)
            {
                pending.push_back(PointerButtonPressed{
                    .button = button, .position = position});
            }
            else
            {
                pending.push_back(PointerButtonReleased{
                    .button = button, .position = position});
            }
        }

        const auto frame = antwika::raylib::frameCount();

        if (frame == wheelFrame)
        {
            return;
        }

        wheelFrame = frame;

        const Vector2 wheel = GetMouseWheelMoveV();

        remainderX += wheel.x;
        remainderY += wheel.y;

        const auto horizontal = static_cast<std::int32_t>(remainderX);
        const auto vertical = static_cast<std::int32_t>(remainderY);

        if (horizontal == 0 && vertical == 0)
        {
            return;
        }

        remainderX -= static_cast<float>(horizontal);
        remainderY -= static_cast<float>(vertical);

        pending.push_back(PointerScrolled{
            .horizontal = horizontal, .vertical = vertical});
    }

}
