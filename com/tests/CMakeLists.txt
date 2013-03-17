make_test(DispatcherTests $<TARGET_OBJECTS:os_com>)
make_test(LocalSerialCommunicationTests)
target_link_libraries(LocalSerialCommunicationTests
    utils
    ${Boost_LIBRARIES}
)
make_test(LocalNetworkCommunicationTests)
target_link_libraries(LocalNetworkCommunicationTests
    utils
    ${Boost_LIBRARIES}
)
