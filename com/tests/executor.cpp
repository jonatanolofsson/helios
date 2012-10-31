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
    EXPECT_EQ(triggered, false);
    os::com::yield('a');
    e.wait(inv);
    EXPECT_EQ(triggered, 1);
    EXPECT_EQ((char)result, 'b');
    e.join();
}
