#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <antwika/gfx/IGfxBackend.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/gfx/WindowEvent.hpp>
#include <antwika/gfx/WindowId.hpp>
#include <antwika/gfx/WindowSpec.hpp>

namespace antwika::gfx::fakes
{

    class FakeClosingBackend final : public IGfxBackend
    {
    public:
        FakeClosingBackend(
            IGfxBackend &innerBackend,
            std::function<bool()> closeWantedGiven)
            : inner(&innerBackend), closeWanted(std::move(closeWantedGiven))
        {
        }

        [[nodiscard]] std::string_view getName() const override
        {
            return "closing";
        }

        [[nodiscard]] std::size_t getMaxWindows() const override
        {
            return inner->getMaxWindows();
        }

        [[nodiscard]] GfxCapabilities getCapabilities() const override
        {
            return inner->getCapabilities();
        }

        [[nodiscard]] std::unique_ptr<IWindow> createWindow(
            const WindowSpec &spec) override
        {
            auto window = inner->createWindow(spec);

            watchedWindowId = window->getId();

            return window;
        }

        [[nodiscard]] std::optional<WindowEvent> pollEvent() override
        {
            const auto event = inner->pollEvent();

            if (event.has_value())
            {
                return event;
            }

            if (!closeSent && closeWanted())
            {
                closeSent = true;

                return WindowEvent{
                    .window = watchedWindowId,
                    .payload = CloseRequested{}};
            }

            return std::nullopt;
        }

    private:
        IGfxBackend *inner;
        std::function<bool()> closeWanted;
        WindowId watchedWindowId = kNullWindowId;
        bool closeSent = false;
    };

}
