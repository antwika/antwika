#pragma once

#include <gmock/gmock.h>

#include <antwika/scheduler/IJob.hpp>

namespace antwika::scheduler::mocks
{

    using antwika::scheduler::IJob;

    class MockJob : public IJob
    {
    public:
        MOCK_METHOD(void, execute, (antwika::time::Tick tick), (override));
    };

}
