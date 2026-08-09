#include "antwika/atlas_editor/EditorSink.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

#include <antwika/app/PointerReading.hpp>
#include <antwika/console/KeyboardLayout.hpp>
#include <antwika/console/Typing.hpp>
#include <antwika/engine/Events.hpp>
#include <antwika/event/Event.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/input/MouseButton.hpp>

#include "antwika/atlas_editor/AtlasEditorError.hpp"
#include "antwika/atlas_editor/AtlasForm.hpp"
#include "antwika/atlas_editor/AtlasMeta.hpp"
#include "antwika/atlas_editor/OpeningSheet.hpp"
#include "antwika/atlas_editor/Canvas.hpp"
#include "antwika/atlas_editor/EditorUi.hpp"
#include "antwika/atlas_editor/FileList.hpp"
#include "antwika/atlas_editor/Ink.hpp"
#include "antwika/atlas_editor/MessageId.hpp"
#include "antwika/atlas_editor/Messages.hpp"
#include "antwika/atlas_editor/Palette.hpp"
#include "antwika/atlas_editor/StatusMessage.hpp"

namespace antwika::atlas_editor
{

    using antwika::app::asPoint;
    using antwika::app::isLeftPress;
    using antwika::app::isLeftRelease;
    using antwika::app::isPressOf;
    using antwika::app::locates;
    using antwika::engine::events::kStop;
    using antwika::event::Event;
    using antwika::input::Key;
    using antwika::input::KeyPressed;
    using antwika::input::MouseButton;
    using antwika::input::PointerButtonPressed;
    using antwika::input::PointerButtonReleased;
    using antwika::input::PointerScrolled;
    using antwika::ui::kNoWidget;

    namespace
    {
        [[nodiscard]] bool isRelease(const InputEvent &event) noexcept
        {
            return std::holds_alternative<PointerButtonReleased>(event);
        }

        [[nodiscard]] const KeyPressed *freshPress(
            const InputEvent &event) noexcept
        {
            const auto *pressed = std::get_if<KeyPressed>(&event);

            if (pressed == nullptr || pressed->repeat)
            {
                return nullptr;
            }

            return pressed;
        }
    }

    EditorSink::EditorSink(
        EditorState &state,
        UiOverlay &overlay,
        IAtlasStore &store,
        const IInputEventCodec &codec,
        const Translator &translator,
        std::optional<std::reference_wrapper<ITickEventSink>> stop)
        : state(state),
          overlay(overlay),
          store(store),
          codec(codec),
          translator(translator),
          stop(stop),
          expectedOpening(openingSheetEvent(state.image()).payload)
    {
    }

    void EditorSink::handle(const TickEvent &event)
    {
        lastTick = event.tick;

        if (event.event.name == events::kOpeningSheet)
        {
            if (event.event.payload != expectedOpening)
            {
                throw AtlasEditorError(
                    "atlas_editor: this replay was recorded against "
                    "another opening sheet (" + event.event.payload
                    + ", and this run opened " + expectedOpening
                    + "); replaying it here would repaint different "
                      "pixels in silence");
            }

            return;
        }

        if (event.event.name == antwika::engine::events::kTick)
        {
            state.noteTick();

            refreshAndAct(false);
            return;
        }

        const auto decoded = codec.decode(event.event);
        if (!decoded.has_value())
        {
            return;
        }

        if (foldedTick != event.tick)
        {
            folded.beginTick();
            foldedTick = event.tick;
        }

        folded.apply(*decoded);

        const Point at = asPoint(folded.mouse().position());

        const Point was = previous.value_or(at);

        if (locates(*decoded))
        {
            previous = at;
        }

        scrollModal(*decoded);

        refreshAndAct(isLeftPress(*decoded), typingNow(*decoded));

        applyToKeyboard(*decoded);

        if (!applyToPreview(*decoded, was, at)
            && !overlay.pointerOverUi())
        {
            applyToSheet(*decoded, was, at);
        }

        if (isRelease(*decoded))
        {
            state.setInkDrag(std::nullopt);
            state.closeStroke();
            draggingPreview = false;
        }
    }

    void EditorSink::applyToKeyboard(const InputEvent &event)
    {
        const KeyPressed *pressed = freshPress(event);

        if (pressed == nullptr)
        {
            return;
        }

        if (pressed->key == Key::Delete
            && state.openModal() == Modal::None)
        {
            state.openStroke();
            state.eraseSelection();
            state.closeStroke();

            return;
        }

        if (!pressed->modifiers.control)
        {
            return;
        }

        if (pressed->key == Key::Z)
        {
            if (pressed->modifiers.shift)
            {
                state.redo();
            }
            else
            {
                state.undo();
            }

            return;
        }

        if (pressed->key == Key::C)
        {
            state.copySelection();
        }
        else if (pressed->key == Key::X)
        {
            state.openStroke();
            state.cutSelection();
            state.closeStroke();
        }
        else if (pressed->key == Key::V)
        {
            state.openStroke();
            state.pasteClipboard();
            state.closeStroke();
        }
    }

    antwika::ui::Keyboard EditorSink::typingNow(const InputEvent &event)
    {
        letters.clear();

        antwika::ui::Keyboard keyboard;

        const KeyPressed *pressed = freshPress(event);

        if (state.openModal() == Modal::None || pressed == nullptr)
        {
            return keyboard;
        }

        if (pressed->key == Key::Enter)
        {
            keyboard.keys.push_back(antwika::ui::Key::Activate);

            return keyboard;
        }

        const auto meaning =
            antwika::console::consoleKeyFor(pressed->key);

        if (meaning.has_value())
        {
            keyboard.keys.push_back(*meaning);
        }

        const char letter = antwika::console::typedCharacterFor(
            pressed->key,
            pressed->modifiers.shift,
            antwika::console::kDefaultKeyboardLayout,
            pressed->modifiers.alt);

        if (letter != '\0')
        {
            letters.assign(1, letter);
            keyboard.keys.push_back(antwika::ui::Key::Character);
            keyboard.typed = letters;
        }

        return keyboard;
    } // GCOVR_EXCL_LINE

    namespace
    {
        [[nodiscard]] PaneRects panesOf(const antwika::ui::WidgetRects &rects)
        {
            return PaneRects{
                .sheet = rects.find(widgets::kSheetPane).value_or(Rect{}),
                .preview = rects.find(widgets::kPreviewPane)};
        }
    }

    Pointer EditorSink::pointerNow(const bool pressed) const
    {
        return Pointer{
            .position = previous,
            .down = folded.mouse().isDown(MouseButton::Left),
            .pressed = pressed};
    }

    void EditorSink::refreshAndAct(
        const bool pressed, const antwika::ui::Keyboard &keyboard)
    {
        auto frame = describeEditor(
            state, pointerNow(pressed), translator, keyboard);
        const auto activated = frame.interactions.activated;
        const bool overUi = frame.interactions.pointerOverUi;

        act(frame.interactions, pressed);

        if (activated != kNoWidget)
        {
            frame = describeEditor(
                state, pointerNow(pressed), translator);
        }

        overlay.set(
            std::move(frame.commands), overUi, panesOf(frame.rects));

        followTheEdit();
    }

    void EditorSink::act(
        const Interactions &interactions, const bool pressed)
    {
        if (actOnModal(interactions, pressed))
        {
            return;
        }

        if (interactions.split.has_value())
        {
            state.setPreviewRatio(interactions.split->ratio);
            state.setPreviewDragging(true);
        }
        else if (!pressed)
        {
            state.setPreviewDragging(false);
        }

        if (interactions.slid.has_value())
        {
            slideInk(*interactions.slid);
            return;
        }

        if (interactions.chosen.has_value())
        {
            choose(state.openMenu(), interactions.chosen->index);
            state.showMenu(Menu::None);

            return;
        }

        const auto activated = interactions.activated;

        if (activated == widgets::kInkButton)
        {
            state.toggleInk();
            return;
        }

        if (activated == widgets::kFileMenu)
        {
            state.showMenu(
                state.openMenu() == Menu::File ? Menu::None : Menu::File);
            return;
        }

        if (activated == widgets::kViewMenu)
        {
            state.showMenu(
                state.openMenu() == Menu::View ? Menu::None : Menu::View);
            return;
        }

        if (pressed)
        {
            state.showMenu(Menu::None);
        }

        for (std::size_t index = 0; index < kToolCount; ++index)
        {
            if (activated
                == widgets::toolWidget(static_cast<Tool>(index)))
            {
                state.selectTool(static_cast<Tool>(index));
                return;
            }
        }

        for (std::size_t index = 0; index < defaultPalette().size();
             ++index)
        {
            if (activated == widgets::swatchWidget(index))
            {
                state.selectColor(index);
                return;
            }
        }
    }

    void EditorSink::slideInk(const SliderChange &slid)
    {
        const auto channel =
            static_cast<std::size_t>(slid.slider)
            - static_cast<std::size_t>(widgets::kFirstInkChannel);

        state.setInkDrag(channel);
        state.setInk(withInkChannel(
            state.color(),
            channel,
            static_cast<std::uint8_t>(slid.value)));
    }

    void EditorSink::choose(const Menu menu, const std::size_t index)
    {
        if (menu == Menu::File)
        {
            chooseFile(static_cast<FileItem>(index));
            return;
        }

        chooseView(static_cast<ViewItem>(index));
    }

    void EditorSink::chooseFile(const FileItem item)
    {
        if (item == FileItem::New)
        {
            state.showNewAtlas();
            return;
        }

        if (item == FileItem::Save)
        {
            state.showModal(
                Modal::Save, state.directory(),
                entriesIn(state.directory()));
            return;
        }

        if (item == FileItem::Load)
        {
            state.showModal(
                Modal::Load, state.directory(),
                entriesIn(state.directory()));
            return;
        }

        quit();
    }

    void EditorSink::chooseView(const ViewItem item)
    {
        const Point middle{
            .x = static_cast<std::int32_t>(state.canvas().width / 2),
            .y = static_cast<std::int32_t>(state.canvas().height / 2)};

        if (item == ViewItem::ZoomIn)
        {
            state.zoomIn(middle);
            return;
        }

        if (item == ViewItem::ZoomOut)
        {
            state.zoomOut(middle);
            return;
        }

        if (item == ViewItem::Fit)
        {
            state.resetView();
            return;
        }

        if (item == ViewItem::Grid)
        {
            state.toggleGrid();
            return;
        }

        if (item == ViewItem::Guides)
        {
            state.toggleGuides();
            return;
        }

        if (item == ViewItem::Preview)
        {
            state.togglePreview();
            return;
        }

        if (item == ViewItem::PreviewFocus)
        {
            state.toggleAutoFocus();
            return;
        }

        if (item == ViewItem::Pivot)
        {
            state.togglePivot();
            return;
        }

        if (item == ViewItem::PixelGrid)
        {
            state.togglePixelGrid();
            return;
        }

        state.togglePointerBorder();
    }

    bool EditorSink::actOnModal(
        const Interactions &interactions, const bool pressed)
    {
        if (state.openModal() == Modal::None)
        {
            return false;
        }

        if (state.openModal() == Modal::New)
        {
            return actOnAtlas(interactions, pressed);
        }

        if (interactions.edit.has_value())
        {
            state.setFileName(
                interactions.edit->text, interactions.edit->cursor);

            if (!interactions.edit->submitted)
            {
                return true;
            }

            confirmFile();
            return true;
        }

        if (!pressed)
        {
            return true;
        }

        const auto activated = interactions.activated;

        if (activated == widgets::kFileConfirm)
        {
            confirmFile();
            return true;
        }

        if (activated == widgets::kFileClose)
        {
            state.closeModal();
            return true;
        }

        for (std::size_t at = 0; at < state.files().size(); ++at)
        {
            if (activated == widgets::fileEntryWidget(at))
            {
                chooseEntry(state.files()[at]);

                return true;
            }
        }

        return true;
    }

    bool EditorSink::actOnAtlas(
        const Interactions &interactions, const bool pressed)
    {
        if (interactions.chosen.has_value())
        {
            state.takePreset(interactions.chosen->index);
            state.showMenu(Menu::None);

            return true;
        }

        if (interactions.edit.has_value())
        {
            state.setFormField(
                interactions.edit->text, interactions.edit->cursor);

            if (interactions.edit->submitted)
            {
                createAtlas();
            }

            return true;
        }

        if (!pressed)
        {
            return true;
        }

        const auto activated = interactions.activated;

        if (activated == widgets::kAtlasCreate)
        {
            createAtlas();
            return true;
        }

        if (activated == widgets::kFileClose)
        {
            state.closeModal();
            return true;
        }

        if (activated == widgets::kAtlasKind)
        {
            state.turnKind();
            return true;
        }

        if (activated == widgets::kAtlasPresets)
        {
            state.showMenu(
                state.openMenu() == Menu::Preset ? Menu::None
                                                 : Menu::Preset);
            return true;
        }

        for (std::size_t field = 0; field < kAtlasFieldCount; ++field)
        {
            if (activated == widgets::atlasFieldWidget(field))
            {
                state.focusField(field);

                return true;
            }
        }

        return true;
    }

    void EditorSink::createAtlas()
    {
        if (!formIsWhole(state.form()))
        {
            report(MessageId::AtlasTooSmall, {});
            return;
        }

        state.openAtlas(metaOf(state.form()));
        state.closeModal();
    }

    void EditorSink::scrollModal(const InputEvent &event)
    {
        if (state.openModal() == Modal::None)
        {
            return;
        }

        const auto *scrolled = std::get_if<PointerScrolled>(&event);

        if (scrolled == nullptr || scrolled->vertical == 0)
        {
            return;
        }

        state.scrollFiles(scrolled->vertical > 0 ? -1 : 1);
    }

    void EditorSink::chooseEntry(const FileEntry &entry)
    {
        if (!entry.directory)
        {
            state.setFileName(entry.name, antwika::ui::kCaretAtEnd);
            return;
        }

        auto walked = pathIn(state.directory(), entry.name);

        state.browse(walked, entriesIn(walked));
    }

    void EditorSink::confirmFile()
    {
        if (state.fileName().empty())
        {
            return;
        }

        const auto named = pathIn(state.directory(), state.fileName());
        const bool saving = state.openModal() == Modal::Save;

        state.closeModal();

        if (saving)
        {
            saveTo(named);
            return;
        }

        loadFrom(named);
    }

    void EditorSink::quit()
    {
        if (!stop.has_value())
        {
            return;
        }

        const Event stopping{.name = kStop}; // GCOVR_EXCL_LINE

        stop->get().handle(
            TickEvent{.tick = lastTick, .event = stopping});
    }

    void EditorSink::strokeAlong(
        const Point from, const Point to, const Brush brush)
    {
        const std::int32_t stepX = to.x < from.x ? -1 : 1;
        const std::int32_t stepY = to.y < from.y ? -1 : 1;

        const std::int32_t spanX = (to.x - from.x) * stepX;
        const std::int32_t spanY = (from.y - to.y) * stepY;

        std::int32_t error = spanX + spanY;
        Point walked = from;

        for (;;)
        {
            (state.*brush)(walked);

            if (walked.x == to.x && walked.y == to.y)
            {
                return;
            }

            const std::int32_t doubled = 2 * error;

            if (doubled >= spanY)
            {
                error += spanY;
                walked.x += stepX;
            }

            if (doubled <= spanX)
            {
                error += spanX;
                walked.y += stepY;
            }
        }
    }

    void EditorSink::applyToSheet(
        const InputEvent &event, const Point was, const Point at)
    {
        if (const auto *scrolled =
                std::get_if<PointerScrolled>(&event);
            scrolled != nullptr)
        {
            if (scrolled->vertical > 0)
            {
                state.zoomIn(at);
            }
            else if (scrolled->vertical < 0)
            {
                state.zoomOut(at);
            }

            return;
        }

        if (!locates(event))
        {
            return;
        }

        state.moveTo(at);

        if (std::holds_alternative<PointerButtonPressed>(event))
        {
            state.openStroke();
        }

        if (isPressOf(event, MouseButton::Right))
        {
            state.clearSelection();
        }

        if (folded.mouse().isDown(MouseButton::Middle))
        {
            state.panBy(Point{.x = at.x - was.x, .y = at.y - was.y});
            return;
        }

        if (state.tool() == Tool::Select)
        {
            applySelection(event, at);
            return;
        }

        if (drawsShape(state.tool())
            && !folded.mouse().isDown(MouseButton::Right))
        {
            applyStroke(event, at);
            return;
        }

        if (folded.mouse().isDown(MouseButton::Left))
        {
            strokeAlong(was, at, &EditorState::applyAt);
        }
        else if (folded.mouse().isDown(MouseButton::Right))
        {
            strokeAlong(was, at, &EditorState::eraseAt);
        }
    }

    bool EditorSink::applyToPreview(
        const InputEvent &event, const Point was, const Point at)
    {
        const auto &pane = overlay.panes().preview;

        if (!pane.has_value())
        {
            return false;
        }

        if (isLeftPress(event) && paneHolds(*pane, at))
        {
            draggingPreview = true;
        }

        if (!draggingPreview && !paneHolds(*pane, at))
        {
            return false;
        }

        if (const auto *scrolled = std::get_if<PointerScrolled>(&event);
            scrolled != nullptr)
        {
            if (scrolled->vertical > 0)
            {
                state.zoomPreviewIn(at);
            }
            else if (scrolled->vertical < 0)
            {
                state.zoomPreviewOut(at);
            }

            return true;
        }

        if (draggingPreview && locates(event))
        {
            state.panPreviewBy(
                Point{.x = at.x - was.x, .y = at.y - was.y});
        }

        return true;
    }

    namespace
    {
        [[nodiscard]] CanvasView wholeSheetView(
            const Rect &pane, const EditorState &state)
        {
            return fittedView(
                pane,
                Rect{.origin = {}, .size = state.image().size()});
        }
    }

    void EditorSink::followTheEdit()
    {
        const auto &pane = overlay.panes().preview;
        const auto &preview = state.preview();

        if (!pane.has_value() || !preview.autoFocus)
        {
            return;
        }

        const auto framed =
            preview.focused.has_value()
                ? viewOfSlot(
                      *pane,
                      state.tiles(),
                      state.image().size(),
                      *preview.focused)
                : std::optional{wholeSheetView(*pane, state)};

        if (framed.has_value())
        {
            state.focusPreviewOn(PreviewPane{.view = *framed});
        }
    }

    void EditorSink::applySelection(
        const InputEvent &event, const Point at)
    {
        if (isLeftPress(event))
        {
            state.beginSelecting(at);
        }
        else if (isLeftRelease(event))
        {
            state.finishSelecting(at);
        }
        else if (folded.mouse().isDown(MouseButton::Left))
        {
            state.dragSelectionTo(at);
        }
    }

    void EditorSink::applyStroke(const InputEvent &event, const Point at)
    {
        if (isLeftPress(event))
        {
            state.beginStroke(at);
        }
        else if (isLeftRelease(event))
        {
            state.finishStroke(at);
        }
        else if (folded.mouse().isDown(MouseButton::Left))
        {
            state.dragStrokeTo(at);
        }
    }

    void EditorSink::report(const MessageId id, std::string detail)
    {
        state.setStatus({.id = id, .detail = std::move(detail)});
    }

    void EditorSink::saveTo(const std::string &path)
    {
        try
        {
            store.saveTo(state.image().bitmap(), path);
            store.saveMetaTo(state.meta(), path);
            state.markSaved();
            state.setStatus({.id = MessageId::Saved, .detail = path});
        }
        catch (const std::runtime_error &failed) // GCOVR_EXCL_LINE
        {
            report(MessageId::SaveFailed, failed.what());
        }
    }

    void EditorSink::loadFrom(const std::string &path)
    {
        try
        {
            auto loaded = store.loadFrom(path);

            if (!loaded.has_value())
            {
                state.setStatus(
                    {.id = MessageId::NothingToLoad,
                     .detail = {}});
                return;
            }

            const auto described = store.loadMetaFrom(path);

            state.replace(Canvas(std::move(*loaded)));

            if (described.has_value())
            {
                state.adoptMeta(*described);
            }

            state.setStatus(
                {.id = MessageId::Loaded, .detail = {}});
        }
        catch (const std::runtime_error &failed) // GCOVR_EXCL_LINE
        {
            report(MessageId::LoadFailed, failed.what());
        }
    }

}
