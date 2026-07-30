#pragma once

#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>

namespace antwika::input::fakes
{

    using antwika::input::IInputBackend;

    /**
     * @brief IInputBackend implementation that reports a scripted queue.
     *
     * What a test uses to stand in for a mouse: hand it the edges a user
     * would have produced and it reports them once each, in order, exactly
     * as a real backend does. No framework, no display, and no timing
     * involved, so a run driven by it is reproducible.
     *
     * A fake rather than a mock, because a scripted queue is the whole
     * behaviour under test in a replay: expressing "these events, then
     * nothing" as call expectations would say more about the polling than
     * about the input.
     */
    class FakeInputBackend final : public IInputBackend
    {
    public:
        /**
         * @brief Construct the backend over the edges it will report.
         * @param events The edges to report, in order.
         * @param capabilities The devices to claim.
         */
        explicit FakeInputBackend(
            std::vector<InputEvent> events = {},
            InputCapabilities capabilities =
                InputCapabilities{.keyboard = true, .pointer = true})
            : events(std::move(events)), devices(capabilities)
        {
        }

        /**
         * @brief Add one more edge to the end of the queue.
         * @param event The edge to report after the ones already queued.
         */
        void push(InputEvent event)
        {
            events.push_back(std::move(event));
        }

        /**
         * @brief Get the backend's name.
         * @return Always "fake".
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "fake";
        }

        /**
         * @brief Get which devices this backend claims.
         * @return Whatever the constructor was told.
         */
        [[nodiscard]] InputCapabilities capabilities() const override
        {
            return devices;
        }

        /**
         * @brief Take the next scripted edge.
         * @return The next edge, or nullopt once the script runs out.
         */
        [[nodiscard]] std::optional<InputEvent> pollEvent() override
        {
            if (next == events.size())
            {
                return std::nullopt;
            }

            auto event = events[next];
            ++next;

            return event;
        }

    private:
        std::vector<InputEvent> events;
        InputCapabilities devices;
        std::size_t next = 0;
    };

} // namespace antwika::input::fakes
