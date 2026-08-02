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
        // X1 and X2 are named after what the window systems call them.
        // Those are exactly the two raylib calls SIDE and EXTRA.
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
    } // namespace

    // The logger is used here and not kept.
    // Nothing after construction has anything to say.
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
        // raylib's input state belongs to a window it has not opened yet.
        if (!IsWindowReady())
        {
            return;
        }

        const auto position = positionNow();

        // The first sample is only where the pointer already was.
        // That is not something that happened.
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

        // The wheel is read once per presented frame, not once per value.
        // raylib holds the same reading for a whole frame, and two honest
        // one-notch frames are indistinguishable from one frame sampled
        // twice; the frame counter is what tells those apart.
        const auto frame = antwika::raylib::frameCount();

        if (frame == wheelFrame)
        {
            return;
        }

        wheelFrame = frame;

        const Vector2 wheel = GetMouseWheelMoveV();

        // Floats, because a touchpad scrolls in fractions of a notch.
        // Truncating each frame would leave such scrolling at zero
        // forever, so the fraction is carried to the next frame instead.
        // Backend-local and upstream of the recorder: a recording holds
        // whole notches either way, and a replay is unaffected.
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

} // namespace antwika::input::raylib
