#pragma once

namespace antwika::ui
{

    class Context;

    /**
     * @brief Keeps one container open for as long as it is alive.
     *
     * The only way to close a container, and the reason Context has no
     * end() of any kind: a mis-nested layout is not something this API
     * can express, so there is no ordering to check.
     *
     * What a scope cannot see is Context::finish() being called inside
     * it, since the call happens while this object is very much alive.
     * That one case is checked there rather than here.
     *
     * Only Context makes one, and only for a container it just opened.
     */
    class Scope final
    {
    public:
        /**
         * @brief Close the container this scope was opened for.
         */
        ~Scope();

        Scope(const Scope &) = delete;
        Scope(Scope &&) = delete;

        Scope &operator=(const Scope &) = delete;
        Scope &operator=(Scope &&) = delete;

    private:
        friend class Context;

        /**
         * @brief Take charge of the container Context has just opened.
         * @param context Must outlive this object.
         */
        explicit Scope(Context &context) noexcept;

        Context &context;
    };

} // namespace antwika::ui
