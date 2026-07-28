#include "antwika/task_worker/TaskSubmissionSink.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/engine/Events.hpp>
#include <antwika/scheduler/Priority.hpp>

#include "antwika/task_worker/Events.hpp"
#include "antwika/task_worker/TaskSubmissionError.hpp"

namespace antwika::task_worker
{

    namespace
    {

        // "id,priority,durationTicks,label[,dependsOnId]"
        constexpr std::size_t kRequiredFieldCount = 4;
        constexpr std::size_t kFieldCountWithDependsOn = 5;

        std::uint64_t parseUInt64(std::string_view text)
        {
            std::uint64_t value{};
            const auto result = std::from_chars(
                text.data(), text.data() + text.size(), value);
            if (result.ec != std::errc{} ||
                result.ptr != text.data() + text.size())
            {
                throw TaskSubmissionError(
                    "TaskSubmissionSink: task.submit payload contains "
                    "a non-numeric or malformed numeric field");
            }
            return value;
        }

        antwika::scheduler::Priority parsePriority(std::string_view text)
        {
            const auto value = parseUInt64(text);
            if (value > std::numeric_limits<std::uint8_t>::max())
            {
                throw TaskSubmissionError(
                    "TaskSubmissionSink: task.submit payload's priority "
                    "must fit in a std::uint8_t (0-255)");
            }
            return static_cast<antwika::scheduler::Priority>(value);
        }

        std::vector<std::string_view> splitByComma(std::string_view text)
        {
            std::vector<std::string_view> tokens;
            std::size_t start = 0;
            while (start <= text.size()) // GCOVR_EXCL_LINE
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
        } // GCOVR_EXCL_LINE

    } // namespace

    TaskSubmissionSink::TaskSubmissionSink(
        World &world,
        SystemScheduler &systemScheduler,
        Scheduler &jobScheduler,
        WorkerLookup &lookup,
        TaskRegistry &registry)
        : world(world),
          systemScheduler(systemScheduler),
          jobScheduler(jobScheduler),
          lookup(lookup),
          registry(registry)
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
        if (tokens.size() != kRequiredFieldCount &&
            tokens.size() != kFieldCountWithDependsOn)
        {
            throw TaskSubmissionError(
                "TaskSubmissionSink: task.submit payload must have 4 "
                "or 5 comma-separated fields "
                "(id,priority,durationTicks,label[,dependsOnId])");
        }

        const auto taskId = parseUInt64(tokens[0]);
        const auto priority = parsePriority(tokens[1]);
        const auto durationTicks = parseUInt64(tokens[2]);
        if (durationTicks == 0)
        {
            throw TaskSubmissionError(
                "TaskSubmissionSink: task.submit payload's "
                "durationTicks must be greater than zero");
        }
        auto label = std::string(tokens[3]);

        std::vector<JobId> dependsOn;
        if (tokens.size() == kFieldCountWithDependsOn)
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
        registry.submit(taskId, job->label(), priority, durationTicks);
        submitted.emplace_back(taskId, jobId);
        jobs.push_back(std::move(job));
    }

} // namespace antwika::task_worker
