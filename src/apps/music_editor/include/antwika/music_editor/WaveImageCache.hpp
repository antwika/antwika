#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/WaveImage.hpp"

namespace antwika::music_editor
{

    /**
     * @brief The rendered waveforms, re-rendered only when they must
     * be.
     *
     * A cycle of audio is a hundred thousand frames, and describing a
     * frame happens on every tick and twice on every event -- so the
     * images are kept here, keyed by the chain text a line reads as
     * and the speed the run is set to, and a refresh whose keys all
     * match renders nothing at all.
     *
     * **Projection state, not simulation state.** Nothing a replay
     * reproduces reads an image: they are pictures of what the score
     * already says, regenerated from it on demand, exactly as a frame
     * is.
     */
    class WaveImageCache final
    {
    public:
        /**
         * @brief Construct the cache over how renders run.
         * @param desc The rate and the cycle length at normal speed.
         */
        explicit WaveImageCache(WaveRenderDesc desc);

        /**
         * @brief Get one image per waveform the score asks for.
         *
         * Parallel to Score::waveforms(), and rendered afresh only
         * for entries whose chain or speed changed since the last
         * refresh.
         *
         * @param score Whose waveforms to image.
         * @param speed Which of kSpeeds the run is set to; one past
         * the table is read as the table's last entry.
         * @return The images, borrowed until the next refresh.
         */
        [[nodiscard]] std::span<const WaveImage> refresh(
            const Score &score, std::size_t speed);

    private:
        // What an image was rendered from.
        // A fresh entry's speed matches nothing a caller can pass.
        struct Key
        {
            std::string chain;
            std::size_t speed = static_cast<std::size_t>(-1);
        };

        WaveRenderDesc desc;
        std::vector<Key> keys;
        std::vector<WaveImage> images;
    };

} // namespace antwika::music_editor
