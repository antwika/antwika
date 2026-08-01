find_package(GTest REQUIRED)

add_subdirectory(conformance)

add_executable(antwika_network_tests
    EndpointTest.cpp
    LoopbackBackendConformanceTest.cpp
    LoopbackBackendTest.cpp
    NetworkCapabilitiesTest.cpp
    NetworkErrorTest.cpp
    NullHostTest.cpp
    NullNetworkBackendConformanceTest.cpp
    NullNetworkBackendTest.cpp
    PacketTest.cpp
    PeerIdTest.cpp
    PortTest.cpp
)

target_link_libraries(antwika_network_tests
    PRIVATE
        antwika::network
        antwika::network::tests::conformance
        antwika::log::tests::mocks
        gtest::gtest
)

antwika_bundle_test(TARGET antwika_network_tests)
