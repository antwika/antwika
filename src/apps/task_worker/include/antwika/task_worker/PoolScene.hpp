#pragma once

#include <cstdint>

#include <antwika/gfx/IRenderer.hpp>
#include <antwika/gfx/Size.hpp>
#include <antwika/i18n/Translator.hpp>

#include "antwika/task_worker/PoolSnapshot.hpp"

namespace antwika::task_worker
{

    using antwika::gfx::IRenderer;
    using antwika::gfx::Size;
    using antwika::i18n::Translator;

    /**
     * @brief Draws a worker pool: the tick's budget, one card per
     * worker, the pending queue in the order it will be pulled, and the
     * tasks that are finished.
     *
     * Stateless and deterministic like apps/life's BoardScene and
     * apps/companion's PetScene: the same snapshot and canvas always
     * produce the same drawing calls in the same order, which is what
     * makes the picture assertable against a mock renderer rather than
     * something to be looked at.
     *
     * **The queue is the point of the picture.** antwika::scheduler
     * promises higher priority first and equal priority in submission
     * order, and that promise is otherwise only visible as an assertion
     * in a test; drawn in the order it will be pulled, with each blocked
     * task named after what it waits for, it is something somebody can
     * watch being kept. The order itself is PoolSnapshot's answer rather
     * than this file's -- a scene that sorted would be a second opinion
     * about the one thing being demonstrated.
     *
     * A worker's bar is (durationTicks - remainingTicks) of
     * durationTicks in whole pixels, so a frame drawn from a snapshot
     * carries no floating-point value at all and two backends cannot
     * round it apart. A task the registry never described has a
     * duration of zero and is drawn with an empty track rather than a
     * divided-by-zero one.
     *
     * Every word comes off an injected i18n::Translator, and a label is
     * not one of them: a task's name is the submission script's own data
     * and is drawn as it arrived. Nothing here is hit-tested -- this
     * application has no pointer input at all -- so what the words
     * measure decides pixels and nothing else.
     */
    class PoolScene final
    {
    public:
        /**
         * @brief Construct the scene over the words it draws with.
         * @param translator Words every caption. Must outlive this
         * scene.
         */
        explicit PoolScene(const Translator &translator);

        /**
         * @brief Draw one frame.
         * @param renderer Receives the drawing calls.
         * @param canvas The size of the area being drawn into. A canvas
         * too small to hold the two columns is cleared and left at
         * that, rather than drawn over itself.
         * @param snapshot What to draw.
         */
        void draw(
            IRenderer &renderer,
            Size canvas,
            const PoolSnapshot &snapshot) const;

    private:
        const Translator &translator;
    };

    /**
     * @brief The narrowest canvas this scene draws anything into.
     */
    inline constexpr std::uint32_t kMinCanvasWidth = 480;

    /**
     * @brief The shortest canvas this scene draws anything into.
     */
    inline constexpr std::uint32_t kMinCanvasHeight = 240;

} // namespace antwika::task_worker
