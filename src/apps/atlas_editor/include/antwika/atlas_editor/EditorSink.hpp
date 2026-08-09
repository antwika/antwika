#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>
#include <antwika/ui/Interactions.hpp>
#include <antwika/ui/Keyboard.hpp>
#include <antwika/ui/Pointer.hpp>
#include <antwika/ui/SliderChange.hpp>
#include <antwika/ui/WidgetId.hpp>

#include "antwika/atlas_editor/EditorState.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/IAtlasStore.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/UiOverlay.hpp"

namespace antwika::atlas_editor
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Point;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputEvent;
    using antwika::input::InputState;
    using antwika::ui::Interactions;
    using antwika::ui::Pointer;
    using antwika::ui::SliderChange;
    using antwika::ui::WidgetId;

    class EditorSink final : public ITickEventSink
    {
    public:
        EditorSink(
            EditorState &state,
            UiOverlay &overlay,
            IAtlasStore &store,
            const IInputEventCodec &codec,
            const Translator &translator,
            std::optional<std::reference_wrapper<ITickEventSink>> stop =
                std::nullopt);

        EditorSink(const EditorSink &) = delete;
        EditorSink(EditorSink &&) = delete;

        EditorSink &operator=(const EditorSink &) = delete;
        EditorSink &operator=(EditorSink &&) = delete;

        void handle(const TickEvent &event) override;

    private:
        [[nodiscard]] Pointer pointerNow(bool pressed) const;

        void refreshAndAct(
            bool pressed, const antwika::ui::Keyboard &keyboard = {});

        [[nodiscard]] antwika::ui::Keyboard typingNow(
            const InputEvent &event);

        void act(const Interactions &interactions, bool pressed);

        void choose(Menu menu, std::size_t index);

        void chooseFile(FileItem item);

        void chooseView(ViewItem item);

        void quit();

        using Brush = void (EditorState::*)(Point);

        void applyToSheet(const InputEvent &event, Point was, Point at);

        [[nodiscard]] bool applyToPreview(
            const InputEvent &event, Point was, Point at);

        void followTheEdit();

        void applySelection(const InputEvent &event, Point at);

        void applyStroke(const InputEvent &event, Point at);

        void applyToKeyboard(const InputEvent &event);

        void strokeAlong(Point from, Point to, Brush brush);

        void report(MessageId id, std::string detail);

        [[nodiscard]] bool actOnModal(
            const Interactions &interactions, bool pressed);

        void slideInk(const SliderChange &slid);

        void scrollModal(const InputEvent &event);

        void chooseEntry(const FileEntry &entry);

        void confirmFile();

        [[nodiscard]] bool actOnAtlas(
            const Interactions &interactions, bool pressed);

        void createAtlas();

        void saveTo(const std::string &path);

        void loadFrom(const std::string &path);

        EditorState &state;
        UiOverlay &overlay;
        IAtlasStore &store;
        const IInputEventCodec &codec;
        const Translator &translator;
        std::optional<std::reference_wrapper<ITickEventSink>> stop;
        bool draggingPreview = false;

        std::string expectedOpening;
        std::string letters;

        InputState folded;
        std::optional<antwika::time::Tick> foldedTick;
        std::optional<Point> previous;
        antwika::time::Tick lastTick = 0;
    };

}
