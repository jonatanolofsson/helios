#include <gtest/gtest.h>
#include <os/com/Executor.hpp>

int triggered = 0;
int result = 0;

void sampleActionFn0(int a, char b) {
    result = a + b;
    ++triggered;
}

TEST(ExecutorTest, ExecuteWhenAllAvailable) {
    result = 0;
    // 1. Set up an action function
    os::Executor<int,char> e(sampleActionFn0);
    int inv = e.getInvokations();
    os::yield(1);
    EXPECT_EQ(triggered, 0);
    os::yield('a');
    e.wait(inv+1);
    EXPECT_EQ(triggered, 1);
    EXPECT_EQ((char)result, 'b');
}


void sampleActionFn1() {
    ++triggered;
}

TEST(ExecutorTest, ExecuteWithNoArgs) {
    result = 0;
    triggered = 0;
    // 1. Set up an action function
    os::Executor<> e(sampleActionFn1);
    int inv = e.getInvokations();
    EXPECT_EQ(triggered, false);
    e.wait(inv+10);
    EXPECT_GT(triggered, 10);
}
