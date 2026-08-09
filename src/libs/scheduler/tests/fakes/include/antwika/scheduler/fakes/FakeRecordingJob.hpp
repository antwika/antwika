#pragma once

#include <string>
#include <utility>
#include <vector>

#include <antwika/time/Tick.hpp>

#include "antwika/scheduler/IJob.hpp"

namespace antwika::scheduler::fakes
{

    class FakeRecordingJob final : public IJob
    {
    public:
        FakeRecordingJob(std::vector<std::string> &log, std::string name)
            : log(log), name(std::move(name))
        {
        }

        void execute(antwika::time::Tick) override
        {
            log.push_back(name);
        }

    private:
        std::vector<std::string> &log;
        std::string name;
    };

}
