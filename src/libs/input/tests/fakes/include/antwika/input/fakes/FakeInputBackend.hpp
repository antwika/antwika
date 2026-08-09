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
            : rounds{std::move(events)}, devices(capabilities)
        {
        }

        explicit FakeInputBackend(
            std::vector<std::vector<InputEvent>> rounds,
            InputCapabilities capabilities =
                InputCapabilities{.keyboard = true, .pointer = true})
            : rounds(std::move(rounds)), devices(capabilities)
        {
        }

        void push(InputEvent event)
        {
            if (rounds.empty())
            {
                rounds.emplace_back();
            }

            rounds.back().push_back(std::move(event));
        }

        [[nodiscard]] std::string_view name() const override
        {
            return "fake";
        }

        [[nodiscard]] InputCapabilities capabilities() const override
        {
            return devices;
        }

        [[nodiscard]] std::optional<InputEvent> pollEvent() override
        {
            if (round == rounds.size())
            {
                return std::nullopt;
            }

            if (next < rounds[round].size())
            {
                auto event = rounds[round][next];
                ++next;

                return event;
            }

            ++round;
            next = 0;

            return std::nullopt;
        }

    private:
        std::vector<std::vector<InputEvent>> rounds;
        InputCapabilities devices;
        std::size_t round = 0;
        std::size_t next = 0;
    };

}
