#pragma once

#include <functional>
#include <string>
#include <string_view>

#include <antwika/gfx/BitmapWindow.hpp>
#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/gfx/WindowDesc.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/log/Level.hpp>
#include <antwika/log/Logger.hpp>
#include <antwika/log/MinimumLevelLogPolicy.hpp>
#include <antwika/log/NullAppender.hpp>
#include <antwika/log/PlainFormatter.hpp>
#include <antwika/time/SystemClock.hpp>

#include "antwika/app/FramePreview.hpp"

namespace antwika::app::preview
{

    struct PreviewDesc final
    {
        std::string_view name;

        std::string_view title;

        antwika::gfx::Size canvas;
    };

    /**
     * @brief Draws a frame onto a page of its own.
     *
     * @param title The window's title.
     * @param canvas The page's size in pixels.
     * @param draw Handed the page's renderer to draw a frame on.
     * @return The page, with the frame drawn on it.
     * @throws antwika::gfx::GfxError If either side of the size is zero.
     */
    [[nodiscard]] inline antwika::gfx::Bitmap drawnPage(
        const std::string_view title,
        const antwika::gfx::Size canvas,
        const std::function<void(antwika::gfx::IRenderer &)> &draw)
    {
        antwika::time::SystemClock clock;
        antwika::log::NullAppender appender;
        antwika::log::PlainFormatter formatter;
        antwika::log::MinimumLevelLogPolicy policy(
            antwika::log::Level::Error);
        antwika::log::Logger logger(formatter, policy, clock, appender);

        antwika::gfx::BitmapWindow window(
            logger,
            antwika::gfx::WindowId{1},
            antwika::gfx::WindowDesc{
                .title = std::string(title), .size = canvas});

        draw(window.renderer());

        return window.page();
    }

    /**
     * @brief Draws onto a page and writes it beside the executable.
     *
     * @param desc The preview's file name, window title and size.
     * @param draw Handed the page's renderer to draw a frame on.
     * @return The path written, as writtenPreview() gives it.
     * @throws antwika::gfx::GfxError If either side of the size is
     *         zero, or the file cannot be written.
     */
    [[nodiscard]] inline std::string drawnPreview(
        const PreviewDesc &desc,
        const std::function<void(antwika::gfx::IRenderer &)> &draw)
    {
        return antwika::app::writtenPreview(
            drawnPage(desc.title, desc.canvas, draw), desc.name);
    }

}
