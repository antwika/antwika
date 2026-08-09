#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>

#include <vector>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/ITexture.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/holdem/Card.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Frame.hpp>
#include <antwika/ui/WidgetId.hpp>
#include <antwika/ui/WidgetRects.hpp>

#include "antwika/poker/SeatSnapshot.hpp"
#include "antwika/poker/TableSnapshot.hpp"

namespace antwika::poker
{

    using antwika::gfx::Color;
    using antwika::gfx::IRenderer;
    using antwika::gfx::ITexture;

    using OptionalAtlas =
        std::optional<std::reference_wrapper<const ITexture>>;
    using antwika::gfx::Rect;
    using antwika::gfx::Size;
    using antwika::ui::Context;
    using antwika::ui::Frame;
    using antwika::ui::WidgetId;
    using antwika::ui::WidgetRects;

    struct ArtBlit final
    {
        Rect source{};

        Rect destination{};

        Color tint{};

        bool operator==(const ArtBlit &other) const = default;
    };

    class TableScene final
    {
    public:
        void draw(
            IRenderer &renderer,
            Size canvas,
            const TableSnapshot &snapshot,
            OptionalAtlas atlas = std::nullopt) const;

        [[nodiscard]] std::vector<ArtBlit> describeArt(
            Size canvas,
            const WidgetRects &rects,
            const TableSnapshot &snapshot) const;

        [[nodiscard]] Frame describe(
            Size canvas, const TableSnapshot &snapshot) const;

    private:
        struct SeatMetrics final
        {
            std::size_t index = 0;
            std::uint32_t width = 0;
            std::uint32_t height = 0;
            std::uint32_t barRoom = 0;
            Chips largestStack = 1;
            std::uint32_t scale = 1;
            bool showButton = false;
            bool handInProgress = false;
        };

        static void appendFelt(
            std::vector<ArtBlit> &art, Size canvas);

        static void appendTable(
            std::vector<ArtBlit> &art, const WidgetRects &rects);

        static void appendBoard(
            std::vector<ArtBlit> &art,
            const WidgetRects &rects,
            const TableSnapshot &snapshot);

        static void appendSeats(
            std::vector<ArtBlit> &art,
            const WidgetRects &rects,
            const TableSnapshot &snapshot);

        static void appendSeat(
            std::vector<ArtBlit> &art,
            const WidgetRects &rects,
            const TableSnapshot &snapshot,
            std::size_t index);

        static void appendCard(
            std::vector<ArtBlit> &art,
            holdem::Card card,
            Rect destination,
            bool faceUp);

        void describeHeader(
            Context &ui, const TableSnapshot &snapshot) const;

        void describeTable(
            Context &ui,
            const TableSnapshot &snapshot,
            std::uint32_t scale) const;

        void describeRing(
            Context &ui,
            const TableSnapshot &snapshot,
            std::uint32_t scale) const;

        void describeSeatRun(
            Context &ui,
            const TableSnapshot &snapshot,
            SeatMetrics metrics,
            std::size_t first,
            std::size_t count,
            bool reversed) const;

        void describeSeat(
            Context &ui,
            const SeatSnapshot &seat,
            SeatMetrics metrics) const;

        void describeBadges(
            Context &ui,
            const SeatSnapshot &seat,
            SeatMetrics metrics) const;

        void describeCards(
            Context &ui,
            std::span<const holdem::Card> cards,
            std::uint32_t scale,
            WidgetId first) const;

        void describeCard(
            Context &ui,
            holdem::Card card,
            std::uint32_t scale,
            WidgetId id) const;
    };

}
