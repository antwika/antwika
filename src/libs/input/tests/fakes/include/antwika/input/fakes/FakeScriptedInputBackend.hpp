#pragma once

#include <chrono>
#include <cstddef>
#include <optional>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputCapabilities.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/Key.hpp>

namespace antwika::input::fakes
{

    class FakeScriptedInputBackend final : public IInputBackend
    {
    public:
        explicit FakeScriptedInputBackend(
            std::vector<std::vector<InputEvent>> tickEventsGiven,
            std::chrono::milliseconds tickPauseGiven =
                std::chrono::milliseconds{0})
            : tickEvents(std::move(tickEventsGiven)),
              tickPause(tickPauseGiven)
        {
        }

        [[nodiscard]] std::string_view getName() const override
        {
            return "scripted";
        }

        [[nodiscard]] InputCapabilities getCapabilities() const override
        {
            return InputCapabilities{.keyboard = true, .pointer = true};
        }

        [[nodiscard]] bool isSpent() const noexcept
        {
            return tickIndex == tickEvents.size();
        }

        [[nodiscard]] std::optional<InputEvent> pollEvent() override
        {
            if (isSpent())
            {
                if (quitKeySent)
                {
                    return std::nullopt;
                }

                quitKeySent = true;

                return InputEvent{KeyPressed{.key = Key::Enter}};
            }

            if (eventIndex < tickEvents[tickIndex].size())
            {
                auto event = tickEvents[tickIndex][eventIndex];

                ++eventIndex;

                return event;
            }

            ++tickIndex;
            eventIndex = 0;

            if (tickPause.count() > 0)
            {
                std::this_thread::sleep_for(tickPause);
            }

            return std::nullopt;
        }

    private:
        std::vector<std::vector<InputEvent>> tickEvents;
        std::chrono::milliseconds tickPause;
        std::size_t tickIndex = 0;
        std::size_t eventIndex = 0;
        bool quitKeySent = false;
    };

}
