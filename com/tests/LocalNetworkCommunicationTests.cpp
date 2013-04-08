#include <gtest/gtest.h>
#include <os/com/PostOffice.hpp>
#include <condition_variable>
#include <mutex>
#include <os/types.hpp>
#include <os/utils/pty.hpp>
#include <os/com/NetworkServer.hpp>
#include <os/com/NetworkClient.hpp>
#include <os/com/TestMessages.hpp>
#include <os/bytemagic.hpp>

#include <unistd.h>

#define LOAD_N                          100000

using namespace os;
using namespace testmessages;

struct Stats_t {
    struct {
        int message0;
        int message1;
        int message2;
    } server, client;
    void reset() {
        server = {0,0,0};
        client = {0,0,0};
    }
} stats;

MyTestMessage0 server2client0 = {'l'};
MyTestMessage0 client2server0 = {'m'};
MyTestMessage1 server2client1 = {'l', 'b'};
MyTestMessage1 client2server1 = {'m', 'b'};
MyTestMessage2 server2client2 = {'l'};
MyTestMessage2 client2server2 = {'m'};

std::mutex serverMessageGuard;
std::mutex clientMessageGuard;
std::condition_variable serverCond;
std::condition_variable clientCond;
typedef os::NetworkServer<8810, testmessages::Messages, 100, 10> Server;
typedef os::NetworkClient<8810, testmessages::Messages, 100, 10> Client;
Server* gserver;
Client* gclient;

void serverMessageHandler0(const U8* msg, const std::size_t len) {
    std::cout << "Local 0" << std::endl;
    std::unique_lock<std::mutex> l(serverMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage0));
    ASSERT_EQ(len, sizeof(MyTestMessage0));
    if(len == sizeof(MyTestMessage0)) {
        MyTestMessage0 message = os::fromBytes<MyTestMessage0>(msg);
        EXPECT_EQ('m', message.a);
        ++stats.server.message0;
        serverCond.notify_all();
    }
}

void clientMessageHandler0(const U8* msg, const std::size_t len) {
    //~ std::cout << "Client 0" << std::endl;
    std::unique_lock<std::mutex> l(clientMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage0));
    ASSERT_EQ(len, sizeof(MyTestMessage0));
    if(len == sizeof(MyTestMessage0)) {
        MyTestMessage0 message = os::fromBytes<MyTestMessage0>(msg);
        EXPECT_EQ('l', message.a);
        ++stats.client.message0;
        clientCond.notify_all();
    }
}

void serverMessageHandler1(const U8* msg, const std::size_t len) {
    //~ std::cout << "Local 1" << std::endl;
    std::unique_lock<std::mutex> l(serverMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage1));
    ASSERT_EQ(len, sizeof(MyTestMessage1));
    if(len == sizeof(MyTestMessage1)) {
        MyTestMessage1 message = os::fromBytes<MyTestMessage1>(msg);
        EXPECT_EQ('m', message.a);
        EXPECT_EQ('b', message.b);
        ++stats.server.message1;
        serverCond.notify_all();
    }
}

void clientMessageHandler1(const U8* msg, const std::size_t len) {
    //~ std::cout << "MCU 1" << std::endl;
    std::unique_lock<std::mutex> l(clientMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage1));
    ASSERT_EQ(len, sizeof(MyTestMessage1));
    if(len == sizeof(MyTestMessage1)) {
        MyTestMessage1 message = os::fromBytes<MyTestMessage1>(msg);
        EXPECT_EQ('l', message.a);
        EXPECT_EQ('b', message.b);
        ++stats.client.message1;
        clientCond.notify_all();
        gclient->send<>(client2server1);
    }
}

void serverMessageHandler2(const U8*, const std::size_t) {
    //~ std::cout << "Server" << std::endl;
    std::unique_lock<std::mutex> l(serverMessageGuard);
    if(++stats.server.message2 == LOAD_N) {
        serverCond.notify_all();
    } else {
        gserver->send<>(server2client2);
    }
}

void clientMessageHandler2(const U8*, const std::size_t) {
    //~ std::cout << "      Client" << std::endl;
    std::unique_lock<std::mutex> l(clientMessageGuard);
    gclient->send<>(client2server2);
    if(++stats.client.message2 == LOAD_N) {
        clientCond.notify_all();
    }
}

class NetworkCommunicationTests : public ::testing::Test {
    public:
        Server server;
        Client client;

        NetworkCommunicationTests()
        : client("localhost")
        {
            server.registerPackager<testmessages::Messages::Id::myTestMessage0>(serverMessageHandler0);
            server.registerPackager<testmessages::Messages::Id::myTestMessage1>(serverMessageHandler1);
            server.registerPackager<testmessages::Messages::Id::myTestMessage2>(serverMessageHandler2);
            client.registerPackager<testmessages::Messages::Id::myTestMessage0>(clientMessageHandler0);
            client.registerPackager<testmessages::Messages::Id::myTestMessage1>(clientMessageHandler1);
            client.registerPackager<testmessages::Messages::Id::myTestMessage2>(clientMessageHandler2);
            stats.reset();
            gserver = &server;
            gclient = &client;
        }
};

void expectMessageCount(const int l0 = 0, const int m0 = 0, const int l1 = 0, const int m1 = 0, const int l2 = 0, const int m2 = 0) {
    EXPECT_EQ(l0, stats.server.message0);
    EXPECT_EQ(m0, stats.client.message0);

    EXPECT_EQ(l1, stats.server.message1);
    EXPECT_EQ(m1, stats.client.message1);

    EXPECT_EQ(l2, stats.server.message2);
    EXPECT_EQ(m2, stats.client.message2);
}

TEST_F(NetworkCommunicationTests, SendAndReceive) {
    std::unique_lock<std::mutex> l(clientMessageGuard);
    server.send<>(server2client0);
    while(!stats.client.message0) clientCond.wait(l);
    expectMessageCount(0, 1);
}

TEST_F(NetworkCommunicationTests, ThereAndBackAgain) {
    std::unique_lock<std::mutex> l(serverMessageGuard);
    server.send<>(server2client1);
    while(stats.server.message1 == 0) serverCond.wait(l);
    expectMessageCount(0, 0, 1, 1);
}

TEST_F(NetworkCommunicationTests, QueueTest) {
    std::unique_lock<std::mutex> l(clientMessageGuard);
    for(int i = 0; i < 15; ++i) {
        server.send<>(server2client0);
    }
    while(stats.client.message0 < 15) clientCond.wait(l);
    expectMessageCount(0, 15);
}

TEST_F(NetworkCommunicationTests, LoadTest) {
    std::unique_lock<std::mutex> l(serverMessageGuard);
    server.send<>(server2client2);
    while(stats.server.message2 < LOAD_N) serverCond.wait(l);
    expectMessageCount(0, 0, 0, 0, LOAD_N, LOAD_N);
}
