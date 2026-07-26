#pragma once

namespace antwika::engine
{

    class IEngine
    {
    public:
        virtual ~IEngine() = default;
        virtual void start() = 0;
    };

} // namespace antwika::engine
