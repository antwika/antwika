#pragma once

#include <vector>

#include <antwika/event/Event.hpp>
#include <antwika/gfx/IWindow.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/Key.hpp>
#include <antwika/simulation/ITickEventSource.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::app
{

    using antwika::event::Event;
    using antwika::gfx::IWindow;
    using antwika::input::IInputEventCodec;
    using antwika::input::Key;
    using antwika::simulation::ITickEventSource;

    /**
     * @brief Makes a nominated key fill the screen with the window, and
     * put it back.
     *
     * A pure observer of the stream, in exactly input::PointerHintSource
     * and app::FramePacedSource's sense: eventsFor() hands back what the
     * source it wraps returned, unchanged and in order. The key press
     * itself is ordinary recorded input like any other, so nothing is
     * added to a recording and nothing is taken out of one.
     *
     * **A fullscreen toggle is an action on the window, not simulation
     * state, and this is where that is said structurally.** It could not
     * live in a sink: a sink is downstream of the recorder, inside the
     * tick path, and everything there is a function of state a replay
     * reproduces. Filling the screen is not -- it changes what
     * IWindow::size() reports and changes nothing else at all, and no
     * layout, no hit test and no simulation may read that number.
     *
     * So a recorded session reaches the same state whether or not
     * anybody pressed the key, and a replay of a session in which
     * somebody did will fill the screen at the same tick and still
     * reach that same state. Which of the two happened is not something
     * the run can tell, and that is the property worth having rather
     * than an accident.
     *
     * **It holds the window rather than an id**, unlike
     * simulation::WindowInputSource, and the difference is deliberate.
     * That class holds an id precisely so it cannot close a window while
     * a renderer is still drawing into it -- see blog/012. Nothing here
     * can close anything: the only calls it makes are
     * IWindow::isFullscreen() and IWindow::setFullscreen(), the window
     * stays alive across both, and both happen while a tick's events are
     * being read, which is before any of that tick's drawing.
     *
     * A repeat is not a fresh press, so holding the key is one toggle
     * rather than a flicker, and several presses inside one tick toggle
     * once each in the order they arrived.
     */
    class FullscreenToggleSource final : public ITickEventSource
    {
    public:
        /**
         * @brief Construct the observer over what it watches and acts on.
         * @param inner The source whose events pass through untouched;
         * must outlive this object.
         * @param window The window to fill the screen with; must outlive
         * this object.
         * @param codec Decodes each event, to recognise the key. Must
         * outlive this object.
         * @param key The key whose press toggles fullscreen.
         */
        FullscreenToggleSource(
            ITickEventSource &inner,
            IWindow &window,
            const IInputEventCodec &codec,
            Key key);

        FullscreenToggleSource(const FullscreenToggleSource &) = delete;
        FullscreenToggleSource(FullscreenToggleSource &&) = delete;

        FullscreenToggleSource &operator=(
            const FullscreenToggleSource &) = delete;
        FullscreenToggleSource &operator=(
            FullscreenToggleSource &&) = delete;

        /**
         * @brief Get a tick's events, unchanged, having acted on any
         * press of the nominated key.
         * @param tick The tick to fetch events for.
         * @return The wrapped source's events, exactly as it returned
         * them.
         * @throws InputError If an input.* event carries a payload of
         * the wrong shape -- raised by the codec, since the wire format
         * is its to police.
         */
        [[nodiscard]] std::vector<Event> eventsFor(
            antwika::time::Tick tick) override;

    private:
        ITickEventSource &inner;
        IWindow &window;
        const IInputEventCodec &codec;
        Key key;
    };

} // namespace antwika::app
