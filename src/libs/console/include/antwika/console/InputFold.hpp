#pragma once

#include <optional>

#include <antwika/event/ITickEventSink.hpp>
#include <antwika/event/TickEvent.hpp>
#include <antwika/gfx/Point.hpp>
#include <antwika/input/IInputEventCodec.hpp>
#include <antwika/input/InputEvent.hpp>
#include <antwika/input/InputState.hpp>
#include <antwika/time/Tick.hpp>

namespace antwika::console
{

    using antwika::event::ITickEventSink;
    using antwika::event::TickEvent;
    using antwika::gfx::Point;
    using antwika::input::IInputEventCodec;
    using antwika::input::InputEvent;
    using antwika::input::InputState;

    /**
     * @brief This app's answer to "what is the pointer doing?", folded
     * once for everything that asks.
     *
     * Every sink that reacts to input is handed every input.* event, so
     * each one could decode it and fold it into an InputState of its own.
     * Several then hold the same truth, kept in step by the order they
     * happen to be registered in, and one of them forgetting a
     * beginTick() would leave two sinks disagreeing about one tick. This
     * is that state, held once.
     *
     * **Register it first**, before anything that reads it: what
     * current() holds is the event the fold was just given, which is the
     * event a later sink is being given now.
     *
     * A tick's edges are cleared when the *next* tick's first event
     * arrives, rather than at the end of the tick that set them. Nothing
     * inside a tick can then read an edge that has already been cleared,
     * whichever order the sinks run in -- which is what the two sinks
     * used to arrange for themselves, in two different places, with a
     * comment each.
     *
     * It is below the recorder like everything else here, so a replay
     * folds the same events again rather than carrying the result.
     */
    class InputFold final : public ITickEventSink
    {
    public:
        /**
         * @brief Construct the fold over the codec that reads the wire.
         * @param codec Decodes the input events off the tick stream. Must
         * outlive this sink.
         */
        explicit InputFold(const IInputEventCodec &codec);

        InputFold(const InputFold &) = delete;
        InputFold(InputFold &&) = delete;

        InputFold &operator=(const InputFold &) = delete;
        InputFold &operator=(InputFold &&) = delete;

        /**
         * @brief Fold a tick event.
         * @param event An input.* event is decoded and folded; anything
         * else only marks the tick it arrived in, which is what clears
         * the previous tick's edges.
         * @throws antwika::input::InputError If an input.* event carries
         * a payload of the wrong shape -- raised by the codec, since the
         * wire format is its to police.
         */
        void handle(const TickEvent &event) override;

        /**
         * @brief Get the input event being handled right now.
         * @return The event this fold last decoded, or nullopt when the
         * last event was not an input.* one.
         */
        [[nodiscard]] const std::optional<InputEvent> &
        current() const noexcept;

        /**
         * @brief Get both devices' folded state.
         * @return The state, with current() already folded in.
         */
        [[nodiscard]] const InputState &state() const noexcept;

        /**
         * @brief Get where the pointer is.
         * @return The folded position, as a point on the canvas.
         */
        [[nodiscard]] Point pointer() const noexcept;

        /**
         * @brief Get where the pointer was before current() was folded.
         *
         * What a drag needs: a pan is the distance between the two, and
         * folding the movement is what loses the place it moved from.
         *
         * @return The position the pointer held one event ago.
         */
        [[nodiscard]] Point pointerBefore() const noexcept;

        /**
         * @brief Check whether anything has said where the pointer is.
         *
         * Nothing has, until an event that locates() it arrives, and the
         * folded default would put it in the canvas's corner where a
         * widget can be -- see PointerReading.hpp.
         *
         * @return True once some folded event has carried a position.
         */
        [[nodiscard]] bool located() const noexcept;

    private:
        const IInputEventCodec &codec;

        InputState folded;
        std::optional<InputEvent> latest;
        std::optional<antwika::time::Tick> foldedTick;

        Point previous{};
        bool hasPosition = false;
    };

} // namespace antwika::console
