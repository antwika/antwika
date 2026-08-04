# Opening, writing and flushing files, stated once.
#
# Four modules had independently written the same eight lines -- open,
# check, write, flush, check -- because no module sat low enough for
# all of them to share: antwika::config owns a copy replay cannot use,
# since config depends on replay, and antwika::app owns a copy config
# cannot use, since app sits at the top of the stack.
#
# Header-only and dependency-free, so everything that touches a file
# can depend on it, and templated on the caller's exception type, so
# the house rule of one exception type per failure category holds --
# this library owns no failure category and throws nothing of its own.
add_library(antwika_io INTERFACE)

add_library(antwika::io ALIAS antwika_io)

target_include_directories(antwika_io
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_io
    EXPORT antwika_ioTargets
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_ioTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
