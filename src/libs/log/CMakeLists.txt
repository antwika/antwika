add_library(antwika_log
    src/Level.cpp
    src/Logger.cpp
    src/MinimumLevelLogPolicy.cpp
    src/NullAppender.cpp
    src/PlainFormatter.cpp
    src/StreamAppender.cpp
)

add_library(antwika::log ALIAS antwika_log)

set_target_properties(antwika_log PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(antwika_log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(antwika_log
    PUBLIC
        antwika::time
)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS antwika_log
    EXPORT antwika_logTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    EXPORT antwika_logTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/antwika
)
