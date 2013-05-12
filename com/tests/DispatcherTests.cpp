#include <gtest/gtest.h>
#include <os/com/Dispatcher.hpp>
#include <os/com/getSignal.hpp>

INSTANTIATE_SIGNAL(int);
INSTANTIATE_SIGNAL(char);

int triggered = 0;
int triggeredB = 0;
int result = 0;

struct SampleActionClass0 {
    void sampleActionFn(int a, char b) {
        result = a + b;
        ++triggered;
    }
    void sampleActionFnB(int) {
        ++triggeredB;
    }
    os::Dispatcher<SampleActionClass0,int,char> e;
    os::Dispatcher<SampleActionClass0,int> e2;
    SampleActionClass0()
        : e(&SampleActionClass0::sampleActionFn, this)
        , e2(&SampleActionClass0::sampleActionFnB, this)
    {}
};

TEST(DispatcherTest, ExecuteWhenAllAvailable) {
    result = 0;
    triggered = 0;
    triggeredB = 0;
    // 1. Set up an action function
    SampleActionClass0 o;
    int inv = o.e.getInvokations();
    int invB= o.e2.getInvokations();
    os::yield(1);
    o.e2.wait(invB+1);
    EXPECT_EQ(triggered, 0);
    EXPECT_EQ(triggeredB, 1);
    os::yield('a');
    o.e.wait(inv+1);
    EXPECT_EQ(triggered, 1);
    EXPECT_EQ((char)result, 'b');
}

struct SampleActionClass1 {
    void sampleActionFn() {
        ++triggered;
    }
    os::Dispatcher<SampleActionClass1> e;
    SampleActionClass1() : e(&SampleActionClass1::sampleActionFn, this) {}
};

TEST(DispatcherTest, ExecuteWithNoArgs) {
    result = 0;
    triggered = 0;
    // 1. Set up an action function
    SampleActionClass1 o;
    int inv = o.e.getInvokations();
    EXPECT_EQ(triggered, false);
    o.e.wait(inv+10);
    EXPECT_GT(triggered, 10);
}
