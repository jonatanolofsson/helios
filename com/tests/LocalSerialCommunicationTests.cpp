#include <gtest/gtest.h>
#include <os/com/PostOffice.hpp>
#include <condition_variable>
#include <mutex>
#include <os/types.hpp>
#include <os/utils/pty.hpp>
#include <os/com/SerialCommunication.hpp>
#include <os/com/TestMessages.hpp>
#include <os/bytemagic.hpp>

#include <unistd.h>

#define LOAD_N                          100000

using namespace os;

struct Stats_t {
    struct {
        int message0;
        int message1;
        int message2;
    } local, mcu;
    void reset() {
        local = {0,0,0};
        mcu = {0,0,0};
    }
} stats;

MyTestMessage0 local2mcu0 = {'l'};
MyTestMessage0 mcu2local0 = {'m'};
MyTestMessage1 local2mcu1 = {'l', 'b'};
MyTestMessage1 mcu2local1 = {'m', 'b'};
MyTestMessage2 local2mcu2 = {'l'};
MyTestMessage2 mcu2local2 = {'m'};

std::mutex localMessageGuard;
std::mutex mcuMessageGuard;
std::condition_variable localCond;
std::condition_variable mcuCond;
typedef SerialCommunication<TestMessages, 100, 10, false> Serial;
Serial* glocal;
Serial* gmcu;

void localMessageHandler0(const U8* msg, const std::size_t len) {
    //~ std::cout << "Local 0" << std::endl;
    std::unique_lock<std::mutex> l(localMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage0));
    ASSERT_EQ(len, sizeof(MyTestMessage0));
    if(len == sizeof(MyTestMessage0)) {
        MyTestMessage0 message = os::fromBytes<MyTestMessage0>(msg);
        EXPECT_EQ('m', message.a);
        ++stats.local.message0;
        localCond.notify_all();
    }
}

void mcuMessageHandler0(const U8* msg, const std::size_t len) {
    //~ std::cout << "MCU 0" << std::endl;
    std::unique_lock<std::mutex> l(mcuMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage0));
    ASSERT_EQ(len, sizeof(MyTestMessage0));
    if(len == sizeof(MyTestMessage0)) {
        MyTestMessage0 message = os::fromBytes<MyTestMessage0>(msg);
        EXPECT_EQ('l', message.a);
        ++stats.mcu.message0;
        mcuCond.notify_all();
    }
}

void localMessageHandler1(const U8* msg, const std::size_t len) {
    //~ std::cout << "Local 1" << std::endl;
    std::unique_lock<std::mutex> l(localMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage1));
    ASSERT_EQ(len, sizeof(MyTestMessage1));
    if(len == sizeof(MyTestMessage1)) {
        MyTestMessage1 message = os::fromBytes<MyTestMessage1>(msg);
        EXPECT_EQ('m', message.a);
        EXPECT_EQ('b', message.b);
        ++stats.local.message1;
        localCond.notify_all();
    }
}

void mcuMessageHandler1(const U8* msg, const std::size_t len) {
    //~ std::cout << "MCU 1" << std::endl;
    std::unique_lock<std::mutex> l(mcuMessageGuard);
    EXPECT_EQ(0, (U64)msg % (U64)alignof(MyTestMessage1));
    ASSERT_EQ(len, sizeof(MyTestMessage1));
    if(len == sizeof(MyTestMessage1)) {
        MyTestMessage1 message = os::fromBytes<MyTestMessage1>(msg);
        EXPECT_EQ('l', message.a);
        EXPECT_EQ('b', message.b);
        ++stats.mcu.message1;
        mcuCond.notify_all();
        gmcu->send<>(mcu2local1);
    }
}

void localMessageHandler2(const U8*, const std::size_t) {
    std::unique_lock<std::mutex> l(localMessageGuard);
    if(++stats.local.message2 == LOAD_N) {
        localCond.notify_all();
    } else {
        glocal->send<>(local2mcu2);
    }
}

void mcuMessageHandler2(const U8*, const std::size_t) {
    std::unique_lock<std::mutex> l(mcuMessageGuard);
    gmcu->send<>(mcu2local2);
    if(++stats.mcu.message2 == LOAD_N) {
        mcuCond.notify_all();
    }
}

class SerialCommunicationTests : public ::testing::Test {
    public:
        os::pty pty;
        Serial local;
        Serial mcu;

        SerialCommunicationTests()
        : pty("local", "mcu")
        , local("local")
        , mcu("mcu")
        {
            local.registerPackager<TestMessages::Id::myTestMessage0>(localMessageHandler0);
            local.registerPackager<TestMessages::Id::myTestMessage1>(localMessageHandler1);
            local.registerPackager<TestMessages::Id::myTestMessage2>(localMessageHandler2);
            mcu.registerPackager<TestMessages::Id::myTestMessage0>(mcuMessageHandler0);
            mcu.registerPackager<TestMessages::Id::myTestMessage1>(mcuMessageHandler1);
            mcu.registerPackager<TestMessages::Id::myTestMessage2>(mcuMessageHandler2);
            stats.reset();
            glocal = &local;
            gmcu = &mcu;
        }
};

void expectMessageCount(const int l0 = 0, const int m0 = 0, const int l1 = 0, const int m1 = 0, const int l2 = 0, const int m2 = 0) {
    EXPECT_EQ(l0, stats.local.message0);
    EXPECT_EQ(m0, stats.mcu.message0);

    EXPECT_EQ(l1, stats.local.message1);
    EXPECT_EQ(m1, stats.mcu.message1);

    EXPECT_EQ(l2, stats.local.message2);
    EXPECT_EQ(m2, stats.mcu.message2);
}

TEST_F(SerialCommunicationTests, SendAndReceive) {
    std::unique_lock<std::mutex> l(mcuMessageGuard);
    local.send<>(local2mcu0);
    while(!stats.mcu.message0) mcuCond.wait(l);
    expectMessageCount(0, 1);
}

TEST_F(SerialCommunicationTests, ThereAndBackAgain) {
    std::unique_lock<std::mutex> l(localMessageGuard);
    local.send<>(local2mcu1);
    while(stats.local.message1 == 0) localCond.wait(l);
    expectMessageCount(0, 0, 1, 1);
}

TEST_F(SerialCommunicationTests, QueueTest) {
    std::unique_lock<std::mutex> l(mcuMessageGuard);
    for(int i = 0; i < 15; ++i) {
        local.send<>(local2mcu0);
    }
    while(stats.mcu.message0 < 15) mcuCond.wait(l);
    expectMessageCount(0, 15);
}

TEST_F(SerialCommunicationTests, LoadTest) {
    std::unique_lock<std::mutex> l(localMessageGuard);
    local.send<>(local2mcu2);
    while(stats.local.message2 < LOAD_N) localCond.wait(l);
    expectMessageCount(0, 0, 0, 0, LOAD_N, LOAD_N);
}
