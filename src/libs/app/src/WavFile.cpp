#include "antwika/app/WavFile.hpp"

#include <fstream>
#include <ios>

#include <antwika/sound/SoundError.hpp>
#include <antwika/sound/WavReader.hpp>

namespace antwika::app
{

    antwika::sound::Waveform readWavFile(
        const std::string &path, std::string_view name)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
        {
            throw antwika::sound::SoundError(
                std::string(name) + ": could not open audio: " + path);
        }

        return antwika::sound::WavReader{}.read(file);
    }

}
