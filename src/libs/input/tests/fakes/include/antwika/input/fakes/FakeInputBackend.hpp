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

    class FakeInputBackend final : public IInputBackend
    {
    public:
        explicit FakeInputBackend(
            std::vector<InputEvent> events = {},
            InputCapabilities capabilities =
                InputCapabilities{.keyboard = true, .pointer = true})
            : roundEvents{std::move(events)}, deviceCapabilities(capabilities)
        {
        }

        explicit FakeInputBackend(
            std::vector<std::vector<InputEvent>> roundEvents,
            InputCapabilities capabilities =
                InputCapabilities{.keyboard = true, .pointer = true})
                                            : roundEvents(
                std::move(roundEvents)), deviceCapabilities(capabilities)
        {
        }

        void push(InputEvent event)
        {
            if (roundEvents.empty())
            {
                roundEvents.emplace_back();
            }

            roundEvents.back().push_back(std::move(event));
        }

        [[nodiscard]] std::string_view name() const override
        {
            return "fake";
        }

        [[nodiscard]] InputCapabilities capabilities() const override
        {
            return deviceCapabilities;
        }

        [[nodiscard]] std::optional<InputEvent> pollEvent() override
        {
            if (roundIndex == roundEvents.size())
            {
                return std::nullopt;
            }

            if (nextIndex < roundEvents[roundIndex].size())
            {
                auto event = roundEvents[roundIndex][nextIndex];
                ++nextIndex;

                return event;
            }

            ++roundIndex;
            nextIndex = 0;

            return std::nullopt;
        }

    private:
        std::vector<std::vector<InputEvent>> roundEvents;
        InputCapabilities deviceCapabilities;
        std::size_t roundIndex = 0;
        std::size_t nextIndex = 0;
    };

}
