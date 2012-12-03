make_test(ExecutorTests)
#~ make_test(PostOfficeTests)
make_test(LocalSerialCommunicationTests)
target_link_libraries(LocalSerialCommunicationTests
    utils
    ${Boost_LIBRARIES}
)
