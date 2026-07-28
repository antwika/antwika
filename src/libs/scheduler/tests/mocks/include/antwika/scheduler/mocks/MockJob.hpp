#pragma once

#include <gmock/gmock.h>

#include <antwika/scheduler/IJob.hpp>

namespace antwika::scheduler::mocks
{

    using antwika::scheduler::IJob;

    /**
     * @brief GMock double for IJob.
     */
    class MockJob : public IJob
    {
    public:
        MOCK_METHOD(void, execute, (antwika::time::Tick tick), (override));
    };

} // namespace antwika::scheduler::mocks
