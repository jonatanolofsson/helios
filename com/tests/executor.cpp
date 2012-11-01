#include <gtest/gtest.h>
#include <os/com/Executor.hpp>
#include <iostream>

int triggered = 0;
int result = 0;

void sampleActionFn0(int a, char b) {
    result = a + b;
    ++triggered;
}

TEST(ExecutorTest, ExecuteWhenAllAvailable) {
    result = 0;
    // 1. Set up an action function
    os::com::Executor<int,char> e(sampleActionFn0, true);
    int inv = e.getInvokations();
    os::com::yield(1);
    EXPECT_EQ(triggered, 0);
    os::com::yield('a');
    e.wait(inv+1);
    EXPECT_EQ(triggered, 1);
    EXPECT_EQ((char)result, 'b');
    e.join();
}


void sampleActionFn1() {
    ++triggered;
}

TEST(ExecutorTest, ExecuteOnceWithNoArgs) {
    result = 0;
    triggered = 0;
    // 1. Set up an action function
    os::com::Executor<> e(sampleActionFn1, true);
    int inv = e.getInvokations();
    e.wait(inv+1);
    EXPECT_EQ(triggered, 1);
    e.join();
}

TEST(ExecutorTest, ExecuteWithNoArgs) {
    result = 0;
    triggered = 0;
    // 1. Set up an action function
    os::com::Executor<> e(sampleActionFn1, false);
    int inv = e.getInvokations();
    EXPECT_EQ(triggered, false);
    e.wait(inv+10);
    EXPECT_GT(triggered, 1);
    e.join();
}
