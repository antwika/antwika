#pragma once

namespace antwika::ui
{

    class Context;

    class ContainerScope final
    {
    public:
        ~ContainerScope();

        ContainerScope(const ContainerScope &) = delete;
        ContainerScope(ContainerScope &&) = delete;

        ContainerScope &operator=(const ContainerScope &) = delete;
        ContainerScope &operator=(ContainerScope &&) = delete;

    private:
        friend class Context;

        explicit ContainerScope(Context &context) noexcept;

        Context &context;
    };

}
