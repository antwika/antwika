#include "antwika/task-worker/TaskSubmissionSink.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task-worker/Events.hpp"
#include "antwika/task-worker/TaskSubmissionError.hpp"

namespace antwika::task_worker
{

    namespace
    {

        std::uint64_t parseUInt64(std::string_view text)
        {
            std::uint64_t value{};
            std::from_chars(text.data(), text.data() + text.size(), value);
            return value;
        }

        std::vector<std::string_view> splitByComma(std::string_view text)
        {
            std::vector<std::string_view> tokens;
            std::size_t start = 0;
            while (start <= text.size())
            {
                const auto separator = text.find(',', start);
                if (separator == std::string_view::npos)
                {
                    tokens.push_back(text.substr(start));
                    break;
                }
                tokens.push_back(text.substr(start, separator - start));
                start = separator + 1;
            }
            return tokens;
        }

    } // namespace

    TaskSubmissionSink::TaskSubmissionSink(
        World &world,
        SystemScheduler &systemScheduler,
        Scheduler &jobScheduler,
        WorkerLookup &lookup)
        : world(world),
          systemScheduler(systemScheduler),
          jobScheduler(jobScheduler),
          lookup(lookup)
    {
    }

    void TaskSubmissionSink::handle(const TimedEvent &event)
    {
        if (event.event.name == antwika::engine::events::kTick)
        {
            world.commit();
            systemScheduler.run(world, event.tick);
            return;
        }

        if (event.event.name != events::kTaskSubmit)
        {
            return;
        }

        const auto tokens = splitByComma(event.event.payload);
        const auto taskId = parseUInt64(tokens[0]);
        const auto priority =
            static_cast<antwika::scheduler::Priority>(parseUInt64(tokens[1]));
        const auto durationTicks = parseUInt64(tokens[2]);
        auto label = std::string(tokens[3]);

        std::vector<JobId> dependsOn;
        if (tokens.size() > 4)
        {
            const auto dependsOnTaskId = parseUInt64(tokens[4]);
            const auto found = std::find_if(
                submitted.begin(),
                submitted.end(),
                [dependsOnTaskId](const auto &entry)
                { return entry.first == dependsOnTaskId; });
            if (found == submitted.end())
            {
                throw TaskSubmissionError(
                    "TaskSubmissionSink: dependsOnId refers to a task "
                    "id that was never submitted");
            }
            dependsOn.push_back(found->second);
        }

        auto job = std::make_unique<TaskJob>(
            lookup, taskId, std::move(label), durationTicks);
        const auto jobId =
            jobScheduler.schedule(*job, priority, dependsOn);
        submitted.emplace_back(taskId, jobId);
        jobs.push_back(std::move(job));
    }

} // namespace antwika::task_worker
