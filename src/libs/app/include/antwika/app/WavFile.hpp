#pragma once

#include <string>
#include <string_view>

#include <antwika/sound/Waveform.hpp>

namespace antwika::app
{

    [[nodiscard]] antwika::sound::Waveform getReadWavFile(
        const std::string &path, std::string_view name);

}
