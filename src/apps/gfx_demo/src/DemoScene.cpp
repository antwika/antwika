#include "antwika/gfx_demo/DemoScene.hpp"

#include <array>
#include <cstdint>
#include <string>

#include <antwika/gfx/Color.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/gfx/Rect.hpp>
#include <antwika/ui/Context.hpp>
#include <antwika/ui/Painter.hpp>
#include <antwika/ui/Sizing.hpp>
#include <antwika/ui/Theme.hpp>

namespace antwika::gfx_demo
{

    using antwika::gfx::Color;
    using antwika::gfx::Point;
    using antwika::gfx::Rect;
    using antwika::ui::Context;
    using antwika::ui::fixedSize;
    using antwika::ui::kFit;
    using antwika::ui::kGrow;
    using antwika::ui::paint;
    using antwika::ui::scaledTheme;
    using antwika::ui::scaleForCanvas;
    using antwika::ui::Theme;

    namespace
    {
        constexpr std::uint32_t kBarCount = 3;

        constexpr Color kBackground{.red = 16, .green = 16, .blue = 24};

        constexpr Color kUntinted{
            .red = 255, .green = 255, .blue = 255, .alpha = 255};

        constexpr Color kWarmTint{
            .red = 255, .green = 96, .blue = 96, .alpha = 255};

        constexpr std::array<Color, kBarCount> kBarColors{
            Color{.red = 224, .green = 64, .blue = 64},
            Color{.red = 64, .green = 224, .blue = 96},
            Color{.red = 80, .green = 128, .blue = 240},
        };

        constexpr std::uint32_t kUnitsAcross = kBarCount * 2 + 1;

        constexpr std::uint32_t kPanelShare = 3;
    }

    void DemoScene::draw(
        IRenderer &renderer,
        Size canvas,
        const ITexture &logo,
        const DrawList &overlay) const
    {
        renderer.clear(kBackground);

        const std::uint32_t unit = canvas.width / kUnitsAcross;
        const std::uint32_t barHeight = canvas.height / 2;
        const auto top = static_cast<std::int32_t>(canvas.height / 4);

        for (std::uint32_t index = 0; index < kBarCount; ++index)
        {
            const auto left = static_cast<std::int32_t>(unit * (index * 2 + 1));

            renderer.drawRect(
                Rect{
                    .origin = {.x = left, .y = top},
                    .size = {.width = unit, .height = barHeight}},
                kBarColors.at(index));
        }

        const Size logoSize = logo.size();
        const std::uint32_t badge = canvas.height / 8;

        const auto centred = static_cast<std::int32_t>(
            canvas.width > badge ? (canvas.width - badge) / 2 : 0);

        renderer.drawTexture(
            logo,
            Rect{.origin = {.x = 0, .y = 0}, .size = logoSize},
            Rect{
                .origin = {.x = centred, .y = static_cast<std::int32_t>(
                                              canvas.height / 16)},
                .size = {.width = badge, .height = badge}},
            kUntinted);

        renderer.drawTexture(
            logo,
            Rect{
                .origin = {.x = 0, .y = 0},
                .size = {
                    .width = logoSize.width / 2,
                    .height = logoSize.height}},
            Rect{
                .origin = {.x = centred, .y = static_cast<std::int32_t>(
                                              canvas.height * 13 / 16)},
                .size = {.width = badge, .height = badge}},
            kWarmTint);

        paint(renderer, overlay);
    }

    Frame DemoScene::describe(
        Size canvas, Pointer pointer, std::uint32_t clicks) const
    {
        Context ui{
            canvas, scaledTheme(Theme{}, scaleForCanvas(canvas)), pointer};

        {
            const auto panel = ui.panel({
                .width = fixedSize(canvas.width / kPanelShare),
                .height = kFit});

            ui.label("Antwika UI");

            {
                const auto body = ui.row({.height = kFit});

                {
                    const auto side =
                        ui.panel({.width = kFit, .height = kGrow});

                    ui.label("layouts", ui.theme().muted);
                    ui.label("buttons", ui.theme().muted);
                    ui.label("text", ui.theme().muted);
                }

                {
                    const auto main = ui.column({.height = kGrow});

                    ui.label("nested rows");
                    ui.label("and columns");

                    ui.label("clicks " + std::to_string(clicks));

                    ui.spacer(kGrow);

                    {
                        const auto actions = ui.row();

                        ui.spacer(kGrow);

                        ui.button("reset", {.id = widgets::kReset});
                        ui.button("count", {.id = widgets::kCount});
                    }
                }
            }
        }

        return ui.finish();
    }

}
