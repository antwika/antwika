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

    inline constexpr std::uint32_t kPollLimit = 1000;

    template <typename BackendTraits>
    class InputBackendConformanceTest : public ::testing::Test
    {
    protected:
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

    TYPED_TEST_SUITE_P(InputBackendConformanceTest);

    TYPED_TEST_P(InputBackendConformanceTest, Name_IsNotEmpty)
    {
        EXPECT_FALSE(this->backend->getName().empty());
    }

    TYPED_TEST_P(InputBackendConformanceTest, Name_DoesNotChange)
    {
        EXPECT_EQ(this->backend->getName(), this->backend->getName());
    }

    TYPED_TEST_P(InputBackendConformanceTest,
                 Capabilities_ClaimAtLeastOneDevice)
    {
        const auto capabilities = this->backend->getCapabilities();

        EXPECT_TRUE(capabilities.keyboard || capabilities.pointer);
    }

    TYPED_TEST_P(InputBackendConformanceTest, Capabilities_DoNotChange)
    {
        EXPECT_EQ(this->backend->getCapabilities(),
                  this->backend->getCapabilities());
    }

    TYPED_TEST_P(InputBackendConformanceTest, PollEvent_DrainsToAnEmptyQueue)
    {
        this->drain();

        SUCCEED();
    }

    TYPED_TEST_P(InputBackendConformanceTest,
                 PollEvent_StaysDrainedWhenRepeated)
    {
        for (std::uint32_t roundIndex = 0; roundIndex < 3; ++roundIndex)
        {
            this->drain();
        }

        SUCCEED();
    }

    TYPED_TEST_P(InputBackendConformanceTest,
                 PollEvent_ReportsNothingUnprompted)
    {
        EXPECT_TRUE(this->drain().empty());
    }

    TYPED_TEST_P(
        InputBackendConformanceTest, PollEvent_ReportsOnlyDevicesItClaims)
    {
        const auto capabilities = this->backend->getCapabilities();

        for (const auto &event : this->drain())
        {
            const bool pointer =
                std::holds_alternative<PointerMoved>(event) ||
                std::holds_alternative<PointerButtonPressed>(event) ||
                std::holds_alternative<PointerButtonReleased>(event) ||
                std::holds_alternative<PointerScrolled>(event);

            if (pointer)
            {
                EXPECT_TRUE(capabilities.pointer) << event.index();
            }
            else
            {
                EXPECT_TRUE(capabilities.keyboard) << event.index();
            }
        }
    }

    REGISTER_TYPED_TEST_SUITE_P(
        InputBackendConformanceTest,
        Name_IsNotEmpty,
        Name_DoesNotChange,
        Capabilities_ClaimAtLeastOneDevice,
        Capabilities_DoNotChange,
        PollEvent_DrainsToAnEmptyQueue,
        PollEvent_StaysDrainedWhenRepeated,
        PollEvent_ReportsNothingUnprompted,
        PollEvent_ReportsOnlyDevicesItClaims);

}
