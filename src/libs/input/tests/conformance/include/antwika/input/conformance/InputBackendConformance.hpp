#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

#include <antwika/input/IInputBackend.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/log/ILogger.hpp>
#include <antwika/log/mocks/MockLogger.hpp>

namespace antwika::input::conformance
{

    using antwika::log::ILogger;
    using antwika::log::mocks::MockLogger;

    /**
     * @brief How many polls a drained queue is allowed to take before the
     * backend is declared to be looping forever.
     */
    inline constexpr std::uint32_t kPollLimit = 1000;

    /**
     * @brief The behaviour every IInputBackend must share, whichever input
     * framework it wraps.
     *
     * Backends under backends/ cannot be held to the coverage gate,
     * because CI has no display and no framework installed. This suite is
     * what replaces that: a backend is finished when it passes this
     * unmodified, which is also the strongest available check that the
     * abstraction is not quietly shaped around one framework.
     * Instantiate it with a traits type exposing
     * static std::unique_ptr<IInputBackend> create(ILogger &):
     *
     * @code
     * INSTANTIATE_TYPED_TEST_SUITE_P(Sdl3, InputBackendConformance, Traits);
     * @endcode
     *
     * Include this header only from a file that instantiates it, since
     * GoogleTest fails a suite that is registered and never instantiated.
     *
     * What is deliberately not asserted matters as much as what is.
     * Nothing here requires that any event ever arrive, because there is
     * no portable way to simulate a keypress, and a headless or unfocused
     * backend is entitled to report nothing. Requiring otherwise would
     * force an honest backend to lie. So the suite asserts invariants
     * rather than reactions.
     */
    template <typename BackendTraits>
    class InputBackendConformance : public ::testing::Test
    {
    protected:
        /**
         * @brief Poll until the queue reports empty.
         *
         * Not [[nodiscard]]: draining is worth doing for its own sake,
         * since a queue that never empties is the failure this suite is
         * most concerned with.
         *
         * @return Every event the backend reported.
         */
        std::vector<InputEvent> drain()
        {
            std::vector<InputEvent> events;

            while (const auto event = backend->pollEvent())
            {
                events.push_back(*event);

                if (events.size() >= kPollLimit)
                {
                    ADD_FAILURE()
                        << "pollEvent never reported an empty queue";
                    break;
                }
            }

            return events;
        }

        ::testing::NiceMock<MockLogger> logger;
        std::unique_ptr<IInputBackend> backend{BackendTraits::create(logger)};
    };

    TYPED_TEST_SUITE_P(InputBackendConformance);

    TYPED_TEST_P(InputBackendConformance, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->name().empty());
    }

    TYPED_TEST_P(InputBackendConformance, Name_DoesNotChange)
    {
        EXPECT_EQ(this->backend->name(), this->backend->name());
    }

    // A backend reporting neither device has nothing to report.
    TYPED_TEST_P(InputBackendConformance, Capabilities_ClaimAtLeastOneDevice)
    {
        const auto capabilities = this->backend->capabilities();

        EXPECT_TRUE(capabilities.keyboard || capabilities.pointer);
    }

    TYPED_TEST_P(InputBackendConformance, Capabilities_DoNotChange)
    {
        EXPECT_EQ(this->backend->capabilities(),
                  this->backend->capabilities());
    }

    TYPED_TEST_P(InputBackendConformance, PollEvent_DrainsToAnEmptyQueue)
    {
        this->drain();

        SUCCEED();
    }

    // This is the invariant a state-diffing backend most often gets wrong.
    // One reading live state has to latch what it already reported.
    // Otherwise the same state comes back on every poll.
    // A caller draining between ticks would then never terminate.
    TYPED_TEST_P(InputBackendConformance, PollEvent_StaysDrainedWhenRepeated)
    {
        for (std::uint32_t round = 0; round < 3; ++round)
        {
            this->drain();
        }

        SUCCEED();
    }

    // Nobody touched anything, so there is nothing to report.
    // An invented edge would put input in a replay nobody produced.
    TYPED_TEST_P(InputBackendConformance, PollEvent_ReportsNothingUnprompted)
    {
        EXPECT_TRUE(this->drain().empty());
    }

    TYPED_TEST_P(
        InputBackendConformance, PollEvent_ReportsOnlyDevicesItClaims)
    {
        const auto capabilities = this->backend->capabilities();

        for (const auto &event : this->drain())
        {
            const bool pointer =
                std::holds_alternative<PointerMoved>(event) ||
                std::holds_alternative<PointerButtonPressed>(event) ||
                std::holds_alternative<PointerButtonReleased>(event) ||
                std::holds_alternative<PointerScrolled>(event);

            if (pointer)
            {
                EXPECT_TRUE(capabilities.pointer)
                    << "reported a pointer event without a pointer";
            }
            else
            {
                EXPECT_TRUE(capabilities.keyboard)
                    << "reported a key event without a keyboard";
            }
        }
    }

    REGISTER_TYPED_TEST_SUITE_P(
        InputBackendConformance,
        Name_IsNotEmpty,
        Name_DoesNotChange,
        Capabilities_ClaimAtLeastOneDevice,
        Capabilities_DoNotChange,
        PollEvent_DrainsToAnEmptyQueue,
        PollEvent_StaysDrainedWhenRepeated,
        PollEvent_ReportsNothingUnprompted,
        PollEvent_ReportsOnlyDevicesItClaims);

} // namespace antwika::input::conformance
