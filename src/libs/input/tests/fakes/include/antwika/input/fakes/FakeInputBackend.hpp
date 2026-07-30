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
     * The script is a list of *rounds*, one per drain. A caller polling
     * until nullopt -- which is what one tick of a run does -- consumes
     * exactly one round, so a script of several rounds spreads its edges
     * over several ticks. That is what makes held state crossing a tick
     * boundary something a test can reach, since it is the part a replay
     * has to regenerate rather than something living and dying inside one
     * tick. A flat script is the one-round case, and the common one.
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
         * @brief Construct the backend over one drain's worth of edges.
         * @param events The edges to report, in order, all in one round.
         * @param capabilities The devices to claim.
         */
        explicit FakeInputBackend(
            std::vector<InputEvent> events = {},
            InputCapabilities capabilities =
                InputCapabilities{.keyboard = true, .pointer = true})
            : rounds{std::move(events)}, devices(capabilities)
        {
        }

        /**
         * @brief Construct the backend over a round per drain.
         * @param rounds The edges to report, one round per drain; an empty
         * round is a tick nothing happened on.
         * @param capabilities The devices to claim.
         */
        explicit FakeInputBackend(
            std::vector<std::vector<InputEvent>> rounds,
            InputCapabilities capabilities =
                InputCapabilities{.keyboard = true, .pointer = true})
            : rounds(std::move(rounds)), devices(capabilities)
        {
        }

        /**
         * @brief Add one more edge to the end of the last round.
         * @param event The edge to report after the ones already queued.
         */
        void push(InputEvent event)
        {
            if (rounds.empty())
            {
                rounds.emplace_back();
            }

            rounds.back().push_back(std::move(event));
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
         * @return The next edge in this round, or nullopt at the end of
         * it, which is also what moves on to the next round.
         */
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

            // Reporting an empty queue is what ends this drain.
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

} // namespace antwika::input::fakes
