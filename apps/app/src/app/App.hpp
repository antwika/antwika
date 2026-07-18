#pragma once

#ifdef _WIN32
#define APP_EXPORT __declspec(dllexport)
#else
#define APP_EXPORT
#endif

namespace antwika::app
{

    class APP_EXPORT App
    {
    public:
        void run();
    };

} // namespace antwika::app
