add_library(log
    src/Level.cpp
    src/Logger.cpp
    src/MinimumLevelLogPolicy.cpp
    src/NullAppender.cpp
    src/PlainFormatter.cpp
    src/StreamAppender.cpp
)

add_library(antwika::log ALIAS log)

set_target_properties(log PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

set(LOG_PUBLIC_HEADERS
    include/antwika/log/IAppender.hpp
    include/antwika/log/IFormatter.hpp
    include/antwika/log/Level.hpp
    include/antwika/log/Logger.hpp
    include/antwika/log/MinimumLevelLogPolicy.hpp
    include/antwika/log/NullAppender.hpp
    include/antwika/log/StreamAppender.hpp
    include/antwika/log/PlainFormatter.hpp
)

set_target_properties(log PROPERTIES
    PUBLIC_HEADER "${LOG_PUBLIC_HEADERS}"
)

target_include_directories(log
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(log PUBLIC antwika::time)

if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(tests)
endif()

install(
    TARGETS log
    EXPORT logTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

install(
    EXPORT logTargets
    NAMESPACE antwika::
    DESTINATION lib/cmake/log
)
