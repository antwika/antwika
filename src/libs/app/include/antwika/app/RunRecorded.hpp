#pragma once

#include <functional>
#include <iostream>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <antwika/cli/CommandLine.hpp>
#include <antwika/cli/FlagSpec.hpp>
#include <antwika/event/IEventSink.hpp>
#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/replay/ReplayCli.hpp>

namespace antwika::app
{

    using antwika::cli::CommandLine;
    using antwika::cli::FlagSpec;
    using antwika::event::IEventSink;
    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::replay::ReplayCliOptions;

    struct RecordedRun final
    {
        const ReplayCliOptions &options;

        const CommandLine &commandLine;

        IEventSink &eventSink;

        std::optional<std::reference_wrapper<ITickEventSink>>
            replayRecorder;
    };

    int runRecorded(
        int argc,
        char **argv,
        std::string_view name,
        const std::function<void(const RecordedRun &)> &body,
        std::span<const FlagSpec> extraFlags = {},
        std::ostream &errors = std::cerr,
        std::ostream &help = std::cout);

    [[nodiscard]] std::vector<TickEvent> scriptedEvents(
        const std::optional<std::string> &replayPath,
        std::string_view fallback = {});

}
