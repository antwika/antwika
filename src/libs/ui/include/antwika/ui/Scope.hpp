#pragma once

namespace antwika::ui
{

    class Context;

    class Scope final
    {
    public:
        ~Scope();

        Scope(const Scope &) = delete;
        Scope(Scope &&) = delete;

        Scope &operator=(const Scope &) = delete;
        Scope &operator=(Scope &&) = delete;

    private:
        friend class Context;

        explicit Scope(Context &context) noexcept;

        Context &context;
    };

}
