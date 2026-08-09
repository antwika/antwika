#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/Bitmap.hpp>
#include <antwika/event/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

#include "antwika/atlas_editor/Canvas.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::Event;
    using antwika::event::ITickEventSource;

    namespace events
    {
        inline constexpr std::string_view kOpeningSheet =
            "atlas.opening_sheet";
    }

    [[nodiscard]] std::uint64_t fingerprintOf(
        const antwika::gfx::Bitmap &image) noexcept;

    [[nodiscard]] Event openingSheetEvent(const Canvas &canvas);

    class OpeningSheetSource final : public ITickEventSource
    {
    public:
        OpeningSheetSource(
            ITickEventSource &inner, std::optional<Event> announcement);

        OpeningSheetSource(const OpeningSheetSource &) = delete;
        OpeningSheetSource(OpeningSheetSource &&) = delete;

        OpeningSheetSource &operator=(const OpeningSheetSource &) =
            delete;
        OpeningSheetSource &operator=(OpeningSheetSource &&) = delete;

        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        std::optional<Event> announcement;
    };

}
