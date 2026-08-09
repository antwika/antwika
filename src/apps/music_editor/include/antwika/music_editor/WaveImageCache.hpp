#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include "antwika/music_editor/Score.hpp"
#include "antwika/music_editor/WaveImage.hpp"

namespace antwika::music_editor
{

    class WaveImageCache final
    {
    public:
        explicit WaveImageCache(WaveRenderDesc desc);

        [[nodiscard]] std::span<const WaveImage> refresh(
            const Score &score, std::size_t speed);

    private:
        struct Key final
        {
            std::string chain;
            std::size_t speed = static_cast<std::size_t>(-1);
        };

        WaveRenderDesc desc;
        std::vector<Key> keys;
        std::vector<WaveImage> images;
    };

}
