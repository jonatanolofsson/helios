#include <gtest/gtest.h>
#include <os/com/Dispatcher.hpp>

int triggered = 0;
int result = 0;

struct SampleActionClass0 {
    void sampleActionFn(int a, char b) {
        result = a + b;
        ++triggered;
    }
    os::Dispatcher<SampleActionClass0,int,char> e;
    SampleActionClass0() : e(&SampleActionClass0::sampleActionFn, this) {}
};

TEST(DispatcherTest, ExecuteWhenAllAvailable) {
    result = 0;
    // 1. Set up an action function
    SampleActionClass0 o;
    int inv = o.e.getInvokations();
    os::yield(1);
    EXPECT_EQ(triggered, 0);
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
