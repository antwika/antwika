#include "antwika/music_editor/WaveImageCache.hpp"

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <utility>

#include <antwika/sequencer/Rational.hpp>

#include "antwika/music_editor/EditorState.hpp"

namespace antwika::music_editor
{

    WaveImageCache::WaveImageCache(WaveRenderDesc desc)
        : desc(std::move(desc))
    {
    }

    std::span<const WaveImage> WaveImageCache::refresh(
        const Score &score, const std::size_t speed)
    {
        const auto &waves = score.waveforms();

        // A wave that is gone takes its image with it.
        // A new one starts on a key no caller can have passed.
        keys.resize(waves.size());
        images.resize(waves.size());

        for (std::size_t at = 0; at < waves.size(); ++at)
        {
            if (keys[at].speed == speed
                && keys[at].chain == waves[at].chain)
            {
                continue;
            }

            keys[at].chain = std::string(waves[at].chain);
            keys[at].speed = speed;

            // One past the table is read as its last entry.
            // The dropdown never passes one, but this stays total.
            const auto &pick =
                kSpeeds[std::min(speed, kSpeeds.size() - 1)];

            images[at] = renderWaveImage(
                waves[at],
                desc,
                sequencer::Rational{
                    pick.numerator, pick.denominator},
                kWaveImageColumns);
        }

        return images;
    }

} // namespace antwika::music_editor
