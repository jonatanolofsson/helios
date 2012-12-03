#include <gtest/gtest.h>
#include <os/com/PostOffice.hpp>

int dispatched = 0;
struct MyTestMessage {
    int a;
    U8 b;
};

MyTestMessage tmsg;

template<int ID> struct TestMessage;
struct TestMessages {
    static const int numberOfMessages = 2;

    enum Id {
        myTestMessage = 0
    };
    template<Id ID>
    struct Message { typedef typename TestMessage<ID>::Type Type; };
};
template<> struct TestMessage<TestMessages::Id::myTestMessage> { typedef MyTestMessage Type; };

typedef os::PostOffice<TestMessages> PostOffice;

void packager(const U8* msg, const std::size_t len) {
    ++dispatched;
    EXPECT_EQ(msg.id, TestMessages::Id::myTestMessage0);
    EXPECT_EQ(msg.size, sizeof(TestMessage));
    memcpy(&tmsg, msg.msg, sizeof(tmsg));
}

class MessageReceiver : public PostOffice {
    public:
        void send(uint32_t id, uint32_t len, U8* m) {
            Message msg = {m, len, id};
            dispatch(msg);
        }
};

TEST(PostOffice, Dispatch) {
    TestMessage m = {1, 'a'};
    MessageReceiver po;
    po.registerPackager(TestMessages::Id::myTestMessage, packager);
    po.send(TestMessages::MY_MSG, sizeof(TestMessage), (U8*)&m);
    EXPECT_EQ(dispatched, 1);
    EXPECT_EQ(tmsg.a, 1);
    EXPECT_EQ(tmsg.b, 'a');
}
