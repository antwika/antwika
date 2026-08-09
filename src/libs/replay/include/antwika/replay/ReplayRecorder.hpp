#pragma once

#include <optional>
#include <ostream>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/geometry/Size.hpp>
#include <antwika/replay/ReplayWriter.hpp>

namespace antwika::replay
{

    class ReplayRecorder final : public event::ITickEventSink
    {
    public:
        explicit ReplayRecorder(
            std::ostream &out,
            std::string destination,
            std::optional<geometry::Size> canvas = std::nullopt);

        ReplayRecorder(const ReplayRecorder &) = delete;
        ReplayRecorder(ReplayRecorder &&) = delete;
        ReplayRecorder &operator=(const ReplayRecorder &) = delete;
        ReplayRecorder &operator=(ReplayRecorder &&) = delete;
        ~ReplayRecorder() override = default;

        void handle(const event::TickEvent &event) override;

    private:
        std::ostream &out;
        std::string destination;
        ReplayWriter writer;
    };

}
