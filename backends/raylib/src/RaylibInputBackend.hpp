#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <string_view>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/MouseButton.hpp>
#include <antwika/input/Position.hpp>
#include <antwika/log/ILogger.hpp>

namespace antwika::input::raylib
{

    using antwika::log::ILogger;

    /**
     * @brief IInputBackend backed by raylib.
     *
     * raylib has no event queue at all: it offers state to inspect, so
     * this is where the edges IInputBackend is expressed in are made.
     * Each sample compares what raylib says now against what was last
     * reported, and turns each difference into one event -- the same
     * latching technique RaylibWindow already uses for a resize, and the
     * invariant the conformance suite's repeated-drain test is really
     * about.
     *
     * That the interface is edges rather than state is what lets this and
     * Sdl3InputBackend implement the same thing, one over a queue and one
     * over globals, instead of the interface being one of them with extra
     * steps.
     *
     * Three limitations, all raylib's, documented rather than hidden:
     *
     * It reports no keyboard. Diffing a key requires a raylib-keycode
     * table this application has no use for yet, and capabilities() says
     * so rather than claiming a device whose events never arrive.
     *
     * Nothing is reported until a window exists, because raylib's input
     * globals are part of the window it has not opened yet.
     *
     * Its state only advances inside EndDrawing, which
     * RaylibRenderer::present() is the only caller of. So events arrive
     * only for an application that presents frames, at most one sample's
     * worth per frame. The wheel is read once per *presented* frame, by
     * the frame counter the renderer bumps at EndDrawing -- by value it
     * cannot be deduplicated, since two honest one-notch frames and one
     * frame sampled twice read exactly the same. Fractions of a notch,
     * which is how a touchpad scrolls, are carried between frames and
     * reported once they add up to a whole one.
     */
    class RaylibInputBackend final : public IInputBackend
    {
    public:
        /**
         * @brief Construct the backend.
         *
         * raylib has no initialisation step of its own for input: the
         * window is the subsystem, so nothing global happens here.
         *
         * @param logger Receives the backend's diagnostics.
         */
        explicit RaylibInputBackend(ILogger &logger);

        RaylibInputBackend(const RaylibInputBackend &) = delete;
        RaylibInputBackend(RaylibInputBackend &&) = delete;

        RaylibInputBackend &operator=(const RaylibInputBackend &) = delete;
        RaylibInputBackend &operator=(RaylibInputBackend &&) = delete;

        /**
         * @brief Get the backend's name.
         * @return Always "raylib".
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Get which devices this backend deals in.
         * @return A pointer and no keyboard.
         */
        [[nodiscard]] InputCapabilities capabilities() const override;

        /**
         * @brief Take the next edge raylib's state implies.
         * @return The next edge, or nullopt when the state matches what
         * was last reported.
         */
        [[nodiscard]] std::optional<InputEvent> pollEvent() override;

    private:
        void sample();

        std::deque<InputEvent> pending;
        std::optional<Position> lastPosition;
        std::array<bool, kMouseButtonCount> held{};

        // Which presented frame the wheel was last read on.
        // Everything before the first present is frame zero, and the
        // sentinel is what lets that first frame still be read once.
        std::uint64_t wheelFrame = ~std::uint64_t{0};

        // The fractions of a notch not yet reported.
        float remainderX = 0.0F;
        float remainderY = 0.0F;
    };

} // namespace antwika::input::raylib
