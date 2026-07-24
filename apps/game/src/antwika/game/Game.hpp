#pragma once

#ifdef _WIN32
#define GAME_EXPORT __declspec(dllexport)
#else
#define GAME_EXPORT
#endif

namespace antwika::game
{

    class GAME_EXPORT Game
    {
    public:
        void run();
    };

} // namespace antwika::game
